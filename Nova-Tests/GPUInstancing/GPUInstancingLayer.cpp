#include "GPUInstancingLayer.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <format>
#include <limits>
#include <random>
#include <span>
#include <string>
#include <unordered_map>

#include <glm/gtc/matrix_transform.hpp>
#include <SDL3/SDL.h>

#include "imgui.h"

#include "Asset/AssetManager.h"
#include "Asset/Assets/MeshAsset.h"
#include "Asset/Assets/ShaderAsset.h"
#include "Core/Application.h"
#include "Core/GraphicsAPI.h"
#include "Core/Log.h"
#include "Math/Camera.h"
#include "Renderer/RHI/RHI_RenderGraphBuilder.h"
#include "Renderer/RHI/RHI_Shaders.h"

using namespace Nova::Core;
using namespace Nova::Core::Asset;
using namespace Nova::Core::Asset::Assets;
using namespace Nova::Core::Events;
using namespace Nova::Core::Math;
using namespace Nova::Core::Renderer::RHI;

constexpr UUID kAABBMeshID = 7; // reserved engine ID, after MeshPrimitive::Torus (6)

constexpr const char* kSuzannePath = "Tests://Meshes/suzanne.obj";
constexpr const char* kDragonPath = "Tests://Meshes/dragon.obj";

std::string FormatGrouped(uint64_t value) {
    std::string digits = std::to_string(value);
    std::string out;
    out.reserve(digits.size() + digits.size() / 3);
    const int first = static_cast<int>(digits.size() % 3);
    if (first > 0)
        out.append(digits, 0, static_cast<size_t>(first));
    for (size_t i = static_cast<size_t>(first); i < digits.size(); i += 3) {
        if (!out.empty())
            out.push_back(' ');
        out.append(digits, i, 3);
    }
    return out;
}

std::string FormatBytes(uint64_t bytes) {
    const double kib = static_cast<double>(bytes) / 1024.0;
    const double mib = kib / 1024.0;
    if (mib >= 1.0)
        return std::format("{:.2f} MiB", mib);
    if (kib >= 1.0)
        return std::format("{:.1f} KiB", kib);
    return std::format("{} B", bytes);
}

AABB ComputeMeshBounds(const RHI_Mesh& mesh) {
    AABB bounds;
    bounds.m_Min = glm::vec3(std::numeric_limits<float>::infinity());
    bounds.m_Max = glm::vec3(-std::numeric_limits<float>::infinity());
    const Vertex* vertices = mesh.GetVertices();
    const uint32_t vertexCount = mesh.GetVertexCount();
    for (uint32_t i = 0; i < vertexCount; ++i)
        bounds.Expand(vertices[i].m_Position);
    if (!std::isfinite(bounds.m_Min.x)) {
        bounds.m_Min = glm::vec3(-0.5f);
        bounds.m_Max = glm::vec3(0.5f);
    }
    return bounds;
}

glm::mat4 MakeObjectModel(const glm::vec3& position, const glm::vec3& axis, float angle, float scale) {
    const float axisLen = glm::length(axis);
    const glm::vec3 n = (axisLen > 1e-6f) ? (axis / axisLen) : glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 model(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, angle, n);
    model = glm::scale(model, glm::vec3(scale));
    return model;
}

/** World-space AABB of a local AABB after `model` (rotation included → box grows, stays axis-aligned). */
AABB TransformAABB(const AABB& local, const glm::mat4& model) {
    const glm::vec3 corners[8] = {
        { local.m_Min.x, local.m_Min.y, local.m_Min.z },
        { local.m_Max.x, local.m_Min.y, local.m_Min.z },
        { local.m_Min.x, local.m_Max.y, local.m_Min.z },
        { local.m_Max.x, local.m_Max.y, local.m_Min.z },
        { local.m_Min.x, local.m_Min.y, local.m_Max.z },
        { local.m_Max.x, local.m_Min.y, local.m_Max.z },
        { local.m_Min.x, local.m_Max.y, local.m_Max.z },
        { local.m_Max.x, local.m_Max.y, local.m_Max.z },
    };

    AABB world;
    bool first = true;
    for (const glm::vec3& corner : corners) {
        const glm::vec3 p = glm::vec3(model * glm::vec4(corner, 1.0f));
        if (first) {
            world = AABB(p, p);
            first = false;
        } else {
            world.Expand(p);
        }
    }
    return world;
}

GPUInstancingLayer::GPUInstancingLayer() : Layer("GPUInstancingLayer") {}

void GPUInstancingLayer::OnAttach() {
    NV_LOG_INFO("GPUInstancingLayer attached");

    if (!CreateRenderer()) {
        NV_LOG_ERROR("GPUInstancingLayer: renderer setup failed");
        return;
    }
    if (!LoadMeshes()) {
        NV_LOG_ERROR("GPUInstancingLayer: failed to load meshes");
        return;
    }

    GenerateMaterials();
    GenerateLights();
    GenerateObjects();
}

void GPUInstancingLayer::OnDetach() {
    NV_LOG_INFO("GPUInstancingLayer detached");

    if (m_Renderer) {
        Application::Get().GetImGuiLayer().DestroyImGuiBackend(GraphicsAPI::Vulkan);
        m_Renderer->Destroy();
        m_Renderer.reset();
    }
}

void GPUInstancingLayer::OnUpdate(float dt) {
    m_DeltaTime = dt;
    m_ElapsedTime += dt;
    PushFrameTimeSample(dt * 1000.0f);

    if (m_Animate) {
        m_AnimationTime += dt;
        m_TransformsDirty = true;
    }

    if (m_AutoOrbit)
        m_CameraYaw += dt * 0.25f;

    if (m_Renderer)
        m_Renderer->Update(dt);
}

void GPUInstancingLayer::OnBegin() {
    if (!m_Renderer)
        return;

    if (m_ResizePending) {
        m_ResizePending = false;
        if (m_PendingWidth > 0 && m_PendingHeight > 0) {
            m_Renderer->Resize(m_PendingWidth, m_PendingHeight);
            if (auto* graph = m_Renderer->GetRenderGraph())
                graph->BindEngineShadowMaps(m_ShadowMaps);
        }
    }

    m_Renderer->BeginFrame();

    const auto t0 = std::chrono::high_resolution_clock::now();
    PrepareInstances();
    auto& window = Application::Get().GetWindow();
    int width = 0, height = 0;
    window.GetWindowSize(width, height);
    UploadFrameData(static_cast<uint32_t>(std::max(width, 1)), static_cast<uint32_t>(std::max(height, 1)));
    const auto t1 = std::chrono::high_resolution_clock::now();
    m_PrepareMs = std::chrono::duration<float, std::milli>(t1 - t0).count();
}

void GPUInstancingLayer::OnRender() {
    if (m_Renderer)
        m_Renderer->RenderFrame();
}

void GPUInstancingLayer::OnEnd() {
    if (m_Renderer)
        m_Renderer->EndFrame();
}

void GPUInstancingLayer::OnImGuiRender() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    void* textureId = (m_Renderer && m_SceneColor.IsValid())
        ? m_Renderer->GetTextureImGuiID(m_SceneColor)
        : nullptr;

    // Fullscreen scene view (same pattern as App's viewport Image — BackgroundDrawList
    // is unreliable with ImGui multi-viewports + Vulkan).
    if (textureId && viewport) {
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        const ImGuiWindowFlags sceneFlags =
            ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus
            | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoScrollbar;
        ImGui::Begin("##GPUInstancingScene", nullptr, sceneFlags);
        const ImVec2 size = ImGui::GetContentRegionAvail();
        if (size.x > 0.0f && size.y > 0.0f)
            ImGui::Image(textureId, size, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
        ImGui::End();
        ImGui::PopStyleVar(3);
    }

    // --- Settings (left) ---
    ImGui::SetNextWindowPos(ImVec2(viewport ? viewport->WorkPos.x + 12.0f : 12.0f,
                                   viewport ? viewport->WorkPos.y + 12.0f : 12.0f),
                            ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(360.0f, 0.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("GPU Instancing - Settings")) {
        if (!textureId)
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f), "Scene texture ImGui ID is null");

        ImGui::SeparatorText("Rendering");
        int mode = m_UseInstancing ? 0 : 1;
        if (ImGui::RadioButton("Instanced (RenderBatches)", &mode, 0))
            m_UseInstancing = true;
        if (ImGui::RadioButton("Per object (RenderInstances)", &mode, 1))
            m_UseInstancing = false;
        ImGui::Checkbox("Animate (rewrites every transform)", &m_Animate);
        ImGui::Checkbox("Auto orbit", &m_AutoOrbit);
        if (ImGui::Checkbox("Show AABBs", &m_ShowAABBs))
            m_BatchesDirty = true;
        ImGui::TextDisabled("Left drag: orbit, wheel: zoom");

        ImGui::SeparatorText("Scene");
        ImGui::SliderInt("Objects", &m_ObjectCount, 1, 100000, "%d", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderInt("Materials", &m_MaterialCount, 1, 4096, "%d", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("Spacing", &m_Spacing, 0.5f, 8.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
        int seed = static_cast<int>(m_Seed);
        if (ImGui::InputInt("Seed", &seed))
            m_Seed = static_cast<uint32_t>(std::max(seed, 0));

        ImGui::SeparatorText("Meshes");
        for (MeshSlot& slot : m_MeshSlots) {
            if (!slot.m_Mesh)
                continue;
            const std::string label = std::format(
                "{}  (asset {}, {} tris)",
                slot.m_Name,
                slot.m_ID,
                FormatGrouped(slot.m_TriangleCount));
            ImGui::Checkbox(label.c_str(), &slot.m_Enabled);
        }

        if (ImGui::Button("Regenerate", ImVec2(-1.0f, 0.0f)))
            RegenerateScene();
    }
    ImGui::End();

    // --- Stats (right) ---
    const float statsWidth = 420.0f;
    ImGui::SetNextWindowPos(
        ImVec2(viewport ? viewport->WorkPos.x + viewport->WorkSize.x - statsWidth - 12.0f : 400.0f,
               viewport ? viewport->WorkPos.y + 12.0f : 12.0f),
        ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(statsWidth, 0.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("GPU Instancing - Stats")) {
        const float frameMs = m_DeltaTime * 1000.0f;
        const float fps = m_DeltaTime > 0.0f ? 1.0f / m_DeltaTime : 0.0f;
        ImGui::Text("%.1f FPS (%.3f ms)", fps, frameMs);

        float avgMs = 0.0f, minMs = 0.0f, maxMs = 0.0f;
        if (m_FrameTimeCount > 0) {
            minMs = std::numeric_limits<float>::infinity();
            maxMs = 0.0f;
            for (size_t i = 0; i < m_FrameTimeCount; ++i) {
                const float sample = m_FrameTimeHistory[i];
                avgMs += sample;
                minMs = std::min(minMs, sample);
                maxMs = std::max(maxMs, sample);
            }
            avgMs /= static_cast<float>(m_FrameTimeCount);
        }
        std::string frameOverlay;
        if (m_FrameTimeCount > 0)
            frameOverlay = std::format("avg {:.2f}  min {:.2f}  max {:.2f}", avgMs, minMs, maxMs);
        ImGui::PlotLines("##frametime",
            m_FrameTimeHistory.data(),
            static_cast<int>(m_FrameTimeCount),
            static_cast<int>(m_FrameTimeCount < kFrameTimeHistory ? 0 : m_FrameTimeWrite),
            frameOverlay.empty() ? nullptr : frameOverlay.c_str(),
            0.0f, maxMs > 0.0f ? maxMs * 1.15f : 1.0f,
            ImVec2(-1.0f, 60.0f));

        ImGui::SeparatorText("Draw");
        ImGui::Text("Mode: %s",
            m_UseInstancing ? "Instanced (one draw per mesh asset)" : "Per object (one draw per instance)");
        ImGui::Text("Draw calls: %s", FormatGrouped(m_DrawCalls).c_str());
        ImGui::Text("Instances drawn: %s", FormatGrouped(m_InstancesDrawn).c_str());
        ImGui::Text("Triangles: %s", FormatGrouped(m_TrianglesDrawn).c_str());
        const uint32_t draws = std::max(m_DrawCalls, 1u);
        ImGui::Text("Instances / draw: %s", FormatGrouped(m_InstancesDrawn / draws).c_str());
        const uint32_t saved = m_UseInstancing && m_InstancesDrawn > m_DrawCalls
            ? (m_InstancesDrawn - m_DrawCalls)
            : 0u;
        ImGui::Text("Draw calls saved by instancing: %s", FormatGrouped(saved).c_str());

        ImGui::SeparatorText("CPU");
        ImGui::Text("Scene pass recording: %.3f ms", m_RecordMs);
        ImGui::Text("Animation (transforms): %.3f ms", m_AnimationMs);
        ImGui::Text("Batcher update: %.3f ms", m_BatcherUpdateMs);
        ImGui::Text("Last batch rebuild: %.3f ms", m_LastBatchRebuildMs);
        ImGui::Text("Batch rebuilds: %u", m_BatchRebuildCount);
        ImGui::Text("Scene generation: %.3f ms", m_SceneGenMs);
        ImGui::TextDisabled("Prepare total: %.3f ms", m_PrepareMs);

        ImGui::SeparatorText("GPU buffers");
        ImGui::Text("Changed/uploaded instances this frame: %s", FormatGrouped(m_UploadedInstances).c_str());
        ImGui::Text("Changed/uploaded materials this frame: %s", FormatGrouped(m_UploadedMaterials).c_str());
        ImGui::Text("Frames in flight: %u", kFramesInFlight);

        uint32_t instanceCapacity = 0;
        uint32_t materialCapacity = 0;
        if (const auto* engine = GetEngine(); engine && m_Renderer) {
            instanceCapacity = m_Renderer->GetGpuBufferElementCount(engine->m_Instances);
            materialCapacity = m_Renderer->GetGpuBufferElementCount(engine->m_Materials);
        }
        const uint64_t instanceUsedBytes =
            static_cast<uint64_t>(m_ObjectInstances.size() + m_AABBInstances.size()) * sizeof(InstanceGPU);
        const uint64_t materialUsedBytes = static_cast<uint64_t>(m_Materials.size()) * sizeof(MaterialGPU);
        const uint64_t instanceRegionBytes = static_cast<uint64_t>(instanceCapacity) * sizeof(InstanceGPU);
        const uint64_t materialRegionBytes = static_cast<uint64_t>(materialCapacity) * sizeof(MaterialGPU);
        const uint64_t totalCapacityBytes =
            (instanceRegionBytes + materialRegionBytes) * static_cast<uint64_t>(kFramesInFlight);

        ImGui::Text("nova.instances used / region: %s / %s", FormatBytes(instanceUsedBytes).c_str(), FormatBytes(instanceRegionBytes).c_str());
        ImGui::Text("nova.materials used / region: %s / %s", FormatBytes(materialUsedBytes).c_str(), FormatBytes(materialRegionBytes).c_str());
        ImGui::Text("Total capacity (all regions): %s", FormatBytes(totalCapacityBytes).c_str());

        const float instanceFrac = instanceCapacity > 0
            ? static_cast<float>(m_ObjectInstances.size() + m_AABBInstances.size()) / static_cast<float>(instanceCapacity)
            : 0.0f;
        const float materialFrac = materialCapacity > 0
            ? static_cast<float>(m_Materials.size()) / static_cast<float>(materialCapacity)
            : 0.0f;
        const std::string instanceBar = std::format("instances {} / {}",
            FormatGrouped(m_ObjectInstances.size() + m_AABBInstances.size()),
            FormatGrouped(instanceCapacity));
        const std::string materialBar = std::format("unique materials {} / {}",
            FormatGrouped(m_Materials.size()),
            FormatGrouped(materialCapacity));
        ImGui::ProgressBar(instanceFrac, ImVec2(-1.0f, 0.0f), instanceBar.c_str());
        ImGui::ProgressBar(materialFrac, ImVec2(-1.0f, 0.0f), materialBar.c_str());

        ImGui::SeparatorText("Batches (grouped by mesh asset ID)");
        ImGui::Text("%u batches for %s objects",
            static_cast<uint32_t>(m_ObjectBatches.size()),
            FormatGrouped(m_Objects.size()).c_str());
        if (ImGui::BeginTable("##batches", 6,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("Mesh");
            ImGui::TableSetupColumn("Asset ID");
            ImGui::TableSetupColumn("Instances");
            ImGui::TableSetupColumn("First");
            ImGui::TableSetupColumn("Tris / inst");
            ImGui::TableSetupColumn("Tris total");
            ImGui::TableHeadersRow();
            for (const InstanceBatch& batch : m_ObjectBatches) {
                const uint32_t trisPerInst = GetMeshTriangleCount(batch.m_MeshID);
                const uint64_t trisTotal = static_cast<uint64_t>(trisPerInst) * batch.m_InstanceCount;
                ImGui::TableNextRow();
                ImGui::TableNextColumn(); ImGui::TextUnformatted(GetMeshName(batch.m_MeshID));
                ImGui::TableNextColumn(); ImGui::Text("%u", batch.m_MeshID);
                ImGui::TableNextColumn(); ImGui::TextUnformatted(FormatGrouped(batch.m_InstanceCount).c_str());
                ImGui::TableNextColumn(); ImGui::Text("%u", batch.m_FirstInstance);
                ImGui::TableNextColumn(); ImGui::TextUnformatted(FormatGrouped(trisPerInst).c_str());
                ImGui::TableNextColumn(); ImGui::TextUnformatted(FormatGrouped(trisTotal).c_str());
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

void GPUInstancingLayer::OnEvent(Event& e) {
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& ev) {
        return OnMouseButtonPressed(ev);
    });
    dispatcher.Dispatch<MouseButtonReleasedEvent>([this](MouseButtonReleasedEvent& ev) {
        return OnMouseButtonReleased(ev);
    });
    dispatcher.Dispatch<MouseMovedEvent>([this](MouseMovedEvent& ev) {
        return OnMouseMoved(ev);
    });
    dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent& ev) {
        return OnMouseScrolled(ev);
    });
    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) {
        return OnWindowResized(ev);
    });
}

bool GPUInstancingLayer::CreateRenderer() {
    auto& app = Application::Get();
    auto& window = app.GetWindow();
    auto& imgui = app.GetImGuiLayer();
    const auto api = GraphicsAPI::Vulkan;

    if (window.GetGraphicsAPI() == GraphicsAPI::SDLRenderer)
        imgui.DestroyImGuiBackend(GraphicsAPI::SDLRenderer);

    if (window.GetGraphicsAPI() != api) {
        if (!window.SetGraphicsAPI(api)) {
            NV_LOG_ERROR("GPUInstancingLayer: failed to set GraphicsAPI::Vulkan");
            return false;
        }
    }
    imgui.SetImGuiBackend(api);

    RHI_SwapchainDesc swapDesc{};
    swapDesc.m_FramesInFlight = kFramesInFlight;
    swapDesc.m_CreateSurface = true;
    swapDesc.m_EnableSwapchain = true;
    swapDesc.m_PreferredPresentMode = RHI_PresentMode::Immediate;

    m_Renderer = IRenderer::Create(api, swapDesc);
    if (!m_Renderer) {
        NV_LOG_ERROR("GPUInstancingLayer: failed to create RHI renderer");
        return false;
    }

    int width = 0, height = 0;
    window.GetWindowSize(width, height);
    if (width <= 0 || height <= 0) {
        width = 1600;
        height = 900;
    }

    auto acquireShader = [](const std::filesystem::path& uri) {
        auto handle = AssetManager::Get().Acquire<ShaderAsset>(uri);
        if (!AssetManager::Get().Load(handle))
            NV_LOG_WARN(("Failed to compile shader: " + uri.generic_string()).c_str());
        return AssetManager::Get().GetAsset(handle);
    };

    auto instancedVert = acquireShader("Engine://Shaders/SceneInstanced.vert.slang");
    auto instancedFrag = acquireShader("Engine://Shaders/SceneInstanced.frag.slang");
    auto vertexColorInstancedFrag = acquireShader("Engine://Shaders/VertexColorInstanced.frag.slang");
    if (!instancedVert || !instancedFrag || !vertexColorInstancedFrag)
        return false;

    RHI_RenderGraphBuilder rg;

    const auto backbuffer = rg.ImportTexture({
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        RHI_TextureFormat::RGBA8,
        RHI_TextureUsage::ColorAttachment,
    }, RHI_ResourceState::Present);

    m_SceneColor = rg.CreateTexture({
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        RHI_TextureFormat::RGBA8,
        RHI_TextureUsage::ColorAttachment | RHI_TextureUsage::Sampled,
    });
    m_SceneDepth = rg.CreateTexture({
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        RHI_TextureFormat::Depth32,
        RHI_TextureUsage::DepthAttachment | RHI_TextureUsage::Sampled,
    });
    m_ShadowMaps = rg.CreateTexture({
        SHADOW_MAP_RESOLUTION,
        SHADOW_MAP_RESOLUTION,
        RHI_TextureFormat::Depth32,
        RHI_TextureUsage::DepthAttachment | RHI_TextureUsage::Sampled,
        MAX_SHADOW_MAPS,
        /*m_ResizeWithViewport*/ false,
        /*m_ComparisonSampler*/ true,
    });

    m_SceneShader = rg.RegisterShader({
        .m_Name = "SceneInstanced",
        .m_Vertex = instancedVert,
        .m_Fragment = instancedFrag,
        .m_VertexLayout = RHI_VertexLayout::Mesh,
    });
    m_AABBShader = rg.RegisterShader({
        .m_Name = "AABBInstanced",
        .m_Vertex = instancedVert,
        .m_Fragment = vertexColorInstancedFrag,
        .m_VertexLayout = RHI_VertexLayout::Mesh,
        .m_PrimitiveTopology = RHI_PrimitiveTopology::Lines,
        .m_DepthTest = false,
        .m_DepthWrite = false,
        .m_CullMode = RHI_CullMode::None,
    });

    // Clear every shadow layer so `nova.shadowMaps` leaves UNDEFINED before the scene samples it.
    // SceneInstanced.frag.slang always samples the array (castShadow==0 early-outs in the shader,
    // but the descriptor still requires SHADER_READ_ONLY_OPTIMAL).
    rg.AddPass("Shadow",
        [&](RHI_PassBuilder& b) {
            b.Write(m_ShadowMaps);
        },
        [this](IPassContext& ctx) {
            RenderShadowPass(ctx);
        });

    rg.AddPass("Scene",
        [&](RHI_PassBuilder& b) {
            b.Write(m_SceneColor);
            b.Write(m_SceneDepth);
            b.Read(m_ShadowMaps);
        },
        [this](IPassContext& ctx) {
            RenderScenePass(ctx);
        });

    rg.AddPass("UI",
        [&](RHI_PassBuilder& b) {
            b.Write(backbuffer);
            b.PresentOnly();
        },
        [](IPassContext& /*ctx*/) {});

    m_Renderer->SetRenderGraph(rg.Build(api));
    if (auto* graph = m_Renderer->GetRenderGraph())
        graph->BindEngineShadowMaps(m_ShadowMaps);

    return m_Renderer->GetRenderGraph() != nullptr;
}

bool GPUInstancingLayer::LoadMeshes() {
    struct Desc {
        const char* m_Path;
        const char* m_Name;
        bool m_EnabledByDefault;
    };
    static constexpr Desc kSlots[] = {
        { "Engine://Primitives/Cube",     "Cube",     true  },
        { "Engine://Primitives/Sphere",   "Sphere",   true  },
        { "Engine://Primitives/Cylinder", "Cylinder", true  },
        { "Engine://Primitives/Capsule",  "Capsule",  true  },
        { "Engine://Primitives/Torus",    "Torus",    true  },
        { kSuzannePath,                   "Suzanne",  true  },
        { kDragonPath,                    "Dragon",   false },
    };
    static_assert(std::size(kSlots) == kMeshSlotCount);

    for (size_t i = 0; i < m_MeshSlots.size(); ++i) {
        auto handle = AssetManager::Get().Acquire<MeshAsset>(kSlots[i].m_Path, MeshAssetDesc{ .m_AABBTreeDepth = 1 });
        if (!AssetManager::Get().Load(handle)) {
            NV_LOG_ERROR(("GPUInstancingLayer: failed to load " + std::string(kSlots[i].m_Path)).c_str());
            return false;
        }
        auto asset = AssetManager::Get().GetAsset(handle);
        if (!asset || !asset->GetCPUMesh()) {
            NV_LOG_ERROR(("GPUInstancingLayer: failed to load " + std::string(kSlots[i].m_Path)).c_str());
            return false;
        }
        m_MeshSlots[i].m_Name = kSlots[i].m_Name;
        m_MeshSlots[i].m_Mesh = asset->GetCPUMesh();
        m_MeshSlots[i].m_ID = handle.GetUUID();
        m_MeshSlots[i].m_LocalBounds = ComputeMeshBounds(*m_MeshSlots[i].m_Mesh);
        m_MeshSlots[i].m_Enabled = kSlots[i].m_EnabledByDefault;
        m_MeshSlots[i].m_TriangleCount = m_MeshSlots[i].m_Mesh->GetIndexCount() / 3u;
        NV_LOG_INFO(std::format("GPUInstancingLayer: {} AssetID = {} ({} tris)",
            kSlots[i].m_Name, m_MeshSlots[i].m_ID, m_MeshSlots[i].m_TriangleCount));
    }

    m_AABBMesh.m_Name = "AABB";
    m_AABBMesh.m_Mesh = CreateUnitAABBWireframeMesh(glm::vec3(0.15f, 0.95f, 0.25f));
    m_AABBMesh.m_ID = kAABBMeshID;
    m_AABBMesh.m_LocalBounds = AABB(glm::vec3(-0.5f), glm::vec3(0.5f));
    m_AABBMesh.m_TriangleCount = 0;
    return static_cast<bool>(m_AABBMesh.m_Mesh);
}

void GPUInstancingLayer::GenerateMaterials() {
    m_MaterialCount = std::max(m_MaterialCount, 1);
    m_Materials.clear();
    m_Materials.reserve(static_cast<size_t>(m_MaterialCount));

    std::mt19937 rng(m_Seed ^ 0x9E3779B9u);
    std::uniform_real_distribution<float> hue(0.0f, 1.0f);
    std::uniform_real_distribution<float> sat(0.45f, 0.95f);
    std::uniform_real_distribution<float> val(0.55f, 1.0f);

    auto hsvToRgb = [](float h, float s, float v) {
        const float i = std::floor(h * 6.0f);
        const float f = h * 6.0f - i;
        const float p = v * (1.0f - s);
        const float q = v * (1.0f - f * s);
        const float t = v * (1.0f - (1.0f - f) * s);
        switch (static_cast<int>(i) % 6) {
        case 0: return glm::vec3(v, t, p);
        case 1: return glm::vec3(q, v, p);
        case 2: return glm::vec3(p, v, t);
        case 3: return glm::vec3(p, q, v);
        case 4: return glm::vec3(t, p, v);
        default: return glm::vec3(v, p, q);
        }
    };

    for (int i = 0; i < m_MaterialCount; ++i) {
        MaterialGPU material{};
        material.m_BaseColor = hsvToRgb(hue(rng), sat(rng), val(rng));
        material.m_Base = 0.85f;
        material.m_SpecularRoughness = 0.35f;
        m_Materials.push_back(material);
    }
    m_PendingUploads = kFramesInFlight;
}

void GPUInstancingLayer::GenerateLights() {
    LightGPU light{};
    light.m_Type = 0; // directional
    light.m_CastShadow = 0;
    light.m_ShadowMapIndex = -1;
    light.m_Intensity = 3.5f;
    light.m_Color = glm::vec3(1.0f);
    light.m_Direction = glm::normalize(glm::vec3(-0.35f, -1.0f, -0.25f));
    m_Lights = { light };
    m_PendingUploads = kFramesInFlight;
}

void GPUInstancingLayer::GenerateObjects() {
    const auto t0 = std::chrono::high_resolution_clock::now();

    m_ObjectCount = std::max(m_ObjectCount, 1);
    if (m_Materials.empty())
        GenerateMaterials();

    std::vector<uint32_t> enabled = GetEnabledMeshIndices();
    if (enabled.empty()) {
        m_MeshSlots[0].m_Enabled = true;
        enabled = GetEnabledMeshIndices();
    }

    m_Objects.resize(static_cast<size_t>(m_ObjectCount));
    m_ObjectModels.resize(m_Objects.size());
    m_ObjectBounds.resize(m_Objects.size());

    m_SceneHalfExtent = std::max(8.0f, std::cbrt(static_cast<float>(m_ObjectCount)) * m_Spacing);
    m_CameraDistance = std::max(m_CameraDistance, m_SceneHalfExtent * 2.2f);

    std::mt19937 rng(m_Seed);
    std::uniform_real_distribution<float> pos(-m_SceneHalfExtent, m_SceneHalfExtent);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    std::uniform_real_distribution<float> scale(0.45f, 1.35f);
    std::uniform_real_distribution<float> speed(-1.2f, 1.2f);
    std::uniform_int_distribution<int> material(0, static_cast<int>(m_Materials.size()) - 1);
    std::uniform_int_distribution<int> meshPick(0, static_cast<int>(enabled.size()) - 1);

    for (SceneObject& object : m_Objects) {
        object.m_Position = glm::vec3(pos(rng), pos(rng), pos(rng));
        const float u = unit(rng);
        const float v = unit(rng);
        const float theta = 2.0f * 3.14159265f * u;
        const float z = 2.0f * v - 1.0f;
        const float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
        object.m_RotationAxis = glm::vec3(r * std::cos(theta), z, r * std::sin(theta));
        object.m_RotationAngle = unit(rng) * 6.2831853f;
        object.m_RotationSpeed = speed(rng);
        object.m_Scale = scale(rng);
        object.m_MaterialIndex = static_cast<uint32_t>(material(rng));
        object.m_MixedMeshSlot = enabled[static_cast<size_t>(meshPick(rng))];
    }

    m_AnimationTime = 0.0f;
    m_TransformsDirty = true;
    m_BatchesDirty = true;

    const auto t1 = std::chrono::high_resolution_clock::now();
    m_SceneGenMs = std::chrono::duration<float, std::milli>(t1 - t0).count();
}

void GPUInstancingLayer::RegenerateScene() {
    m_BatchRebuildCount = 0;
    GenerateMaterials();
    GenerateLights();
    GenerateObjects();
}

void GPUInstancingLayer::UpdateTransforms() {
    const auto t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < m_Objects.size(); ++i) {
        SceneObject& object = m_Objects[i];
        if (m_Animate)
            object.m_RotationAngle += object.m_RotationSpeed * m_DeltaTime;

        m_ObjectModels[i] = MakeObjectModel(
            object.m_Position, object.m_RotationAxis, object.m_RotationAngle, object.m_Scale);

        // World AABB: transform local corners, then take min/max (stays axis-aligned).
        m_ObjectBounds[i] = TransformAABB(GetObjectMesh(object).m_LocalBounds, m_ObjectModels[i]);
    }
    m_TransformsDirty = false;
    m_BatchesDirty = true;
    const auto t1 = std::chrono::high_resolution_clock::now();
    m_AnimationMs = std::chrono::duration<float, std::milli>(t1 - t0).count();
}

void GPUInstancingLayer::RebuildBatches() {
    const auto t0 = std::chrono::high_resolution_clock::now();
    auto finishBatches = [](uint32_t firstInstance,
                            std::vector<InstanceGPU>& instances,
                            std::vector<InstanceBatch>& batches,
                            std::vector<uint32_t>& instanceBatch) {
        uint32_t offset = firstInstance;
        for (auto& batch : batches) {
            batch.m_FirstInstance = offset;
            offset += batch.m_InstanceCount;
        }

        if (std::is_sorted(instanceBatch.begin(), instanceBatch.end()))
            return;

        std::vector<uint32_t> cursors(batches.size());
        for (size_t b = 0; b < batches.size(); ++b)
            cursors[b] = batches[b].m_FirstInstance - firstInstance;

        std::vector<InstanceGPU> scratch(instances.size());
        for (size_t i = 0; i < instances.size(); ++i) {
            const uint32_t dst = cursors[instanceBatch[i]]++;
            scratch[dst] = instances[i];
        }
        instances.swap(scratch);
    };

    m_ObjectInstances.clear();
    m_ObjectBatches.clear();
    {
        std::unordered_map<UUID, uint32_t> batchByMesh;
        std::vector<uint32_t> instanceBatch;
        uint32_t lastBatch = 0;
        m_ObjectInstances.reserve(m_Objects.size());
        instanceBatch.reserve(m_Objects.size());

        for (size_t i = 0; i < m_Objects.size(); ++i) {
            const MeshSlot& slot = GetObjectMesh(m_Objects[i]);
            if (!slot.m_Mesh)
                continue;
            if (m_ObjectBatches.empty() || m_ObjectBatches[lastBatch].m_MeshID != slot.m_ID) {
                const auto [it, inserted] =
                    batchByMesh.try_emplace(slot.m_ID, static_cast<uint32_t>(m_ObjectBatches.size()));
                if (inserted)
                    m_ObjectBatches.push_back({ slot.m_ID, slot.m_Mesh, 0, 0 });
                lastBatch = it->second;
            }
            ++m_ObjectBatches[lastBatch].m_InstanceCount;
            InstanceGPU& instance = m_ObjectInstances.emplace_back();
            instance.m_Model = m_ObjectModels[i];
            instance.m_MaterialIndex = m_Objects[i].m_MaterialIndex;
            instanceBatch.push_back(lastBatch);
        }
        finishBatches(0, m_ObjectInstances, m_ObjectBatches, instanceBatch);
    }

    m_AABBInstances.clear();
    m_AABBBatches.clear();
    m_AABBFirstInstance = static_cast<uint32_t>(m_ObjectInstances.size());
    if (m_ShowAABBs && m_AABBMesh.m_Mesh) {
        std::unordered_map<UUID, uint32_t> batchByMesh;
        std::vector<uint32_t> instanceBatch;
        uint32_t lastBatch = 0;
        m_AABBInstances.reserve(m_Objects.size());
        instanceBatch.reserve(m_Objects.size());

        for (size_t i = 0; i < m_Objects.size(); ++i) {
            // m_ObjectBounds is already world-space and axis-aligned — no object rotation here.
            if (m_AABBBatches.empty() || m_AABBBatches[lastBatch].m_MeshID != m_AABBMesh.m_ID) {
                const auto [it, inserted] =
                    batchByMesh.try_emplace(m_AABBMesh.m_ID, static_cast<uint32_t>(m_AABBBatches.size()));
                if (inserted)
                    m_AABBBatches.push_back({ m_AABBMesh.m_ID, m_AABBMesh.m_Mesh, 0, 0 });
                lastBatch = it->second;
            }
            ++m_AABBBatches[lastBatch].m_InstanceCount;
            InstanceGPU& instance = m_AABBInstances.emplace_back();
            instance.m_Model = AABBToModelMatrix(m_ObjectBounds[i]);
            instance.m_MaterialIndex = 0;
            instanceBatch.push_back(lastBatch);
        }
        finishBatches(m_AABBFirstInstance, m_AABBInstances, m_AABBBatches, instanceBatch);
    }

    m_BatchesDirty = false;
    m_PendingUploads = kFramesInFlight;
    ++m_BatchRebuildCount;
    const auto t1 = std::chrono::high_resolution_clock::now();
    m_LastBatchRebuildMs = std::chrono::duration<float, std::milli>(t1 - t0).count();
    m_BatcherUpdateMs = m_LastBatchRebuildMs;
}

void GPUInstancingLayer::PrepareInstances() {
    const auto* engine = GetEngine();
    if (!engine || !m_Renderer)
        return;

    m_UploadedInstances = 0;
    m_UploadedMaterials = 0;
    m_AnimationMs = 0.0f;
    m_BatcherUpdateMs = 0.0f;

    if (m_TransformsDirty)
        UpdateTransforms();
    if (m_BatchesDirty)
        RebuildBatches();

    const uint32_t instanceCount =
        static_cast<uint32_t>(m_ObjectInstances.size() + m_AABBInstances.size());
    const bool grownInstances = GrowEngineBuffer(engine->m_Instances, std::max(instanceCount, 1u));
    const bool grownMaterials = GrowEngineBuffer(engine->m_Materials, std::max(static_cast<uint32_t>(m_Materials.size()), 1u));
    if (grownInstances || grownMaterials)
        m_PendingUploads = kFramesInFlight;

    if (m_PendingUploads > 0) {
        if (!m_ObjectInstances.empty()) {
            UpdateStructuredBufferRange<InstanceGPU>(
                *m_Renderer, engine->m_Instances, std::span<const InstanceGPU>(m_ObjectInstances), 0);
            m_UploadedInstances += static_cast<uint32_t>(m_ObjectInstances.size());
        }
        if (!m_AABBInstances.empty()) {
            UpdateStructuredBufferRange<InstanceGPU>(
                *m_Renderer, engine->m_Instances, std::span<const InstanceGPU>(m_AABBInstances), m_AABBFirstInstance);
            m_UploadedInstances += static_cast<uint32_t>(m_AABBInstances.size());
        }
        if (!m_Materials.empty()) {
            UpdateStructuredBufferRange<MaterialGPU>(
                *m_Renderer, engine->m_Materials, std::span<const MaterialGPU>(m_Materials), 0);
            m_UploadedMaterials = static_cast<uint32_t>(m_Materials.size());
        }
        if (engine->m_Scene.m_Lights.IsValid() && !m_Lights.empty()) {
            m_Renderer->UpdateGpuBuffer(
                engine->m_Scene.m_Lights,
                m_Lights.data(),
                sizeof(LightGPU) * m_Lights.size(),
                0);
        }
        --m_PendingUploads;
    }

    if (!engine->m_Scene.m_Uniforms.IsValid())
        return;

    int width = 0, height = 0;
    Application::Get().GetWindow().GetWindowSize(width, height);
    const float aspect = (height > 0) ? static_cast<float>(width) / static_cast<float>(height) : 16.0f / 9.0f;

    Camera camera(
        GetCameraPosition(),
        glm::vec3(0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        45.0f,
        aspect,
        0.1f,
        std::max(1000.0f, m_SceneHalfExtent * 8.0f + m_CameraDistance * 4.0f),
        true);

    SceneUniforms scene{};
    scene.m_Camera.m_View = camera.GetViewMatrix();
    scene.m_Camera.m_Proj = camera.GetProjectionMatrix();
    scene.m_Camera.m_ViewProj = scene.m_Camera.m_Proj * scene.m_Camera.m_View;
    scene.m_Camera.m_InvViewProj = glm::inverse(scene.m_Camera.m_ViewProj);
    scene.m_Camera.m_Position = camera.m_LookFrom;
    scene.m_LightCount = static_cast<int>(m_Lights.size());
    m_Renderer->UpdateGpuBuffer(engine->m_Scene.m_Uniforms, &scene, sizeof(scene), 0);
}

bool GPUInstancingLayer::GrowEngineBuffer(RHI_GpuBufferHandle handle, uint32_t elementCount) {
    if (!m_Renderer || !handle.IsValid())
        return false;
    const uint32_t capacity = m_Renderer->GetGpuBufferElementCount(handle);
    if (capacity == 0 || elementCount <= capacity)
        return false;
    if (!EnsureGpuBufferCapacity(*m_Renderer, handle, elementCount)) {
        NV_LOG_WARN("GPUInstancingLayer: failed to grow an engine GPU buffer");
        return false;
    }
    return true;
}

void GPUInstancingLayer::RenderShadowPass(IPassContext& ctx) {
    if (!m_ShadowMaps.IsValid())
        return;

    for (uint32_t layer = 0; layer < MAX_SHADOW_MAPS; ++layer) {
        ctx.BeginDepthLayer(m_ShadowMaps, layer, true);
        ctx.EndDepthLayer();
    }
}

void GPUInstancingLayer::RenderScenePass(IPassContext& ctx) {
    const auto t0 = std::chrono::high_resolution_clock::now();
    m_DrawCalls = 0;
    m_InstancesDrawn = 0;
    m_TrianglesDrawn = 0;
    m_DrawCalls += DrawBatches(ctx, m_SceneShader, m_ObjectBatches, RHI_PrimitiveTopology::Triangles);
    if (m_ShowAABBs)
        m_DrawCalls += DrawBatches(ctx, m_AABBShader, m_AABBBatches, RHI_PrimitiveTopology::Lines);
    const auto t1 = std::chrono::high_resolution_clock::now();
    m_RecordMs = std::chrono::duration<float, std::milli>(t1 - t0).count();
}

void GPUInstancingLayer::UploadFrameData(uint32_t width, uint32_t height) {
    auto* graph = m_Renderer ? m_Renderer->GetRenderGraph() : nullptr;
    if (!graph)
        return;

    const glm::vec3 resolution(static_cast<float>(width), static_cast<float>(height), 1.0f);
    auto setFrame = [&](IShaders* shader) {
        if (!shader)
            return;
        shader->SetParameter("m_Time", m_ElapsedTime);
        shader->SetParameter("m_TimeDelta", m_DeltaTime);
        shader->SetParameter("m_FrameRate", m_DeltaTime > 0.0f ? 1.0f / m_DeltaTime : 0.0f);
        shader->SetParameter("m_Frame", m_FrameIndex);
        shader->SetParameter("m_Resolution", resolution);
    };
    setFrame(graph->GetShader(m_SceneShader));
    setFrame(graph->GetShader(m_AABBShader));
    ++m_FrameIndex;
}

uint32_t GPUInstancingLayer::DrawBatches(IPassContext& ctx,
                                         RHI_ShaderHandle shader,
                                         const std::vector<InstanceBatch>& batches,
                                         RHI_PrimitiveTopology topology) {
    if (!shader.IsValid() || batches.empty())
        return 0;

    ctx.BindShader(shader);

    uint32_t draws = 0;
    for (const InstanceBatch& batch : batches) {
        if (!batch.m_Mesh || batch.m_InstanceCount == 0)
            continue;

        RHI_DrawIndexedCommand cmd{};
        cmd.m_Mesh = batch.m_Mesh;
        cmd.m_Topology = topology;
        cmd.m_IndexType = RHI_IndexType::UInt32;
        cmd.m_IndexCount = batch.m_Mesh->GetIndexCount();

        const uint32_t trisPerInst = batch.m_Mesh->GetIndexCount() / 3u;
        if (m_UseInstancing) {
            cmd.m_InstanceCount = batch.m_InstanceCount;
            cmd.m_FirstInstance = batch.m_FirstInstance;
            ctx.DrawIndexed(cmd);
            ++draws;
            m_InstancesDrawn += batch.m_InstanceCount;
            m_TrianglesDrawn += static_cast<uint64_t>(trisPerInst) * batch.m_InstanceCount;
        } else {
            for (uint32_t i = 0; i < batch.m_InstanceCount; ++i) {
                cmd.m_InstanceCount = 1;
                cmd.m_FirstInstance = batch.m_FirstInstance + i;
                ctx.DrawIndexed(cmd);
                ++draws;
            }
            m_InstancesDrawn += batch.m_InstanceCount;
            m_TrianglesDrawn += static_cast<uint64_t>(trisPerInst) * batch.m_InstanceCount;
        }
    }
    return draws;
}

const RHI_EngineParameterBlock* GPUInstancingLayer::GetEngine() const {
    if (!m_Renderer)
        return nullptr;
    auto* graph = m_Renderer->GetRenderGraph();
    return graph ? graph->GetEngineParameterBlock() : nullptr;
}

const GPUInstancingLayer::MeshSlot& GPUInstancingLayer::GetObjectMesh(const SceneObject& object) const {
    const uint32_t slot = std::min(object.m_MixedMeshSlot, static_cast<uint32_t>(m_MeshSlots.size() - 1));
    if (m_MeshSlots[slot].m_Mesh && m_MeshSlots[slot].m_Enabled)
        return m_MeshSlots[slot];

    for (const MeshSlot& candidate : m_MeshSlots) {
        if (candidate.m_Enabled && candidate.m_Mesh)
            return candidate;
    }
    return m_MeshSlots[0];
}

const char* GPUInstancingLayer::GetMeshName(UUID meshID) const {
    for (const MeshSlot& slot : m_MeshSlots) {
        if (slot.m_ID == meshID)
            return slot.m_Name;
    }
    if (m_AABBMesh.m_ID == meshID)
        return m_AABBMesh.m_Name;
    return "Unknown";
}

uint32_t GPUInstancingLayer::GetMeshTriangleCount(UUID meshID) const {
    for (const MeshSlot& slot : m_MeshSlots) {
        if (slot.m_ID == meshID)
            return slot.m_TriangleCount;
    }
    return 0;
}

std::vector<uint32_t> GPUInstancingLayer::GetEnabledMeshIndices() const {
    std::vector<uint32_t> enabled;
    enabled.reserve(m_MeshSlots.size());
    for (uint32_t i = 0; i < static_cast<uint32_t>(m_MeshSlots.size()); ++i) {
        if (m_MeshSlots[i].m_Enabled && m_MeshSlots[i].m_Mesh)
            enabled.push_back(i);
    }
    return enabled;
}

void GPUInstancingLayer::PushFrameTimeSample(float ms) {
    m_FrameTimeHistory[m_FrameTimeWrite] = ms;
    m_FrameTimeWrite = (m_FrameTimeWrite + 1) % kFrameTimeHistory;
    m_FrameTimeCount = std::min(m_FrameTimeCount + 1, kFrameTimeHistory);
}

glm::vec3 GPUInstancingLayer::GetCameraPosition() const {
    const float cp = std::cos(m_CameraPitch);
    return glm::vec3(
        m_CameraDistance * std::cos(m_CameraYaw) * cp,
        m_CameraDistance * std::sin(m_CameraPitch),
        m_CameraDistance * std::sin(m_CameraYaw) * cp);
}

bool GPUInstancingLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e) {
    if (ImGui::GetIO().WantCaptureMouse)
        return false;
    if (e.GetMouseButton() == SDL_BUTTON_LEFT) {
        m_Orbiting = true;
        return true;
    }
    return false;
}

bool GPUInstancingLayer::OnMouseButtonReleased(MouseButtonReleasedEvent& e) {
    if (e.GetMouseButton() == SDL_BUTTON_LEFT) {
        m_Orbiting = false;
        return true;
    }
    return false;
}

bool GPUInstancingLayer::OnMouseMoved(MouseMovedEvent& e) {
    const glm::vec2 pos(static_cast<float>(e.GetX()), static_cast<float>(e.GetY()));
    const glm::vec2 delta = pos - m_LastMouse;
    m_LastMouse = pos;
    if (!m_Orbiting || ImGui::GetIO().WantCaptureMouse)
        return false;

    m_CameraYaw += delta.x * 0.005f;
    m_CameraPitch = std::clamp(m_CameraPitch + delta.y * 0.005f, -1.4f, 1.4f);
    return true;
}

bool GPUInstancingLayer::OnMouseScrolled(MouseScrolledEvent& e) {
    if (ImGui::GetIO().WantCaptureMouse)
        return false;
    m_CameraDistance = std::clamp(m_CameraDistance * std::pow(0.9f, static_cast<float>(e.GetYOffset())), 2.0f, 4000.0f);
    return true;
}

bool GPUInstancingLayer::OnWindowResized(WindowResizeEvent& e) {
    m_PendingWidth = static_cast<int>(e.GetWidth());
    m_PendingHeight = static_cast<int>(e.GetHeight());
    m_ResizePending = true;
    return false;
}