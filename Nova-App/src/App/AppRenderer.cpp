#include "App/AppRenderer.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <span>

#include "Asset/AssetManager.h"
#include "Asset/Assets/MeshAsset.h"
#include "Asset/Assets/ShaderAsset.h"
#include "Core/Assert.h"
#include "Core/Log.h"
#include "ECS/Components/LightComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/MeshRendererComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "Math/AABB.h"
#include "Math/Light.h"
#include "Scene/SceneQuery.h"

namespace Nova::App {

    using namespace Nova::Core::Asset;
    using namespace Nova::Core::Asset::Assets;
    using namespace Nova::Core::ECS::Components;
    using namespace Nova::Core::Math;

    void AppRenderer::Initialize(Nova::Core::GraphicsAPI api, uint32_t width, uint32_t height) {
        Nova::Core::Renderer::RHI::RHI_SwapchainDesc swapDesc{};
        swapDesc.m_FramesInFlight = 3;
        swapDesc.m_CreateSurface = true;
        swapDesc.m_EnableSwapchain = true;
        swapDesc.m_PreferredPresentMode = Nova::Core::Renderer::RHI::RHI_PresentMode::Default;
        m_Renderer = Nova::Core::Renderer::RHI::IRenderer::Create(api, swapDesc);
        NV_ASSERT_MSG(m_Renderer, "Renderer is not initialized.");

        namespace RG = Nova::Core::Renderer::RHI;
        RG::RHI_RenderGraphBuilder fg;

        const auto backbuffer = fg.ImportTexture({
            width,
            height,
            RG::RHI_TextureFormat::RGBA8,
            RG::RHI_TextureUsage::ColorAttachment,
        }, RG::RHI_ResourceState::Present);

        RG::RHI_TextureHandle color = fg.CreateTexture({
            width,
            height,
            RG::RHI_TextureFormat::RGBA8,
            RG::RHI_TextureUsage::ColorAttachment | RG::RHI_TextureUsage::Sampled,
        });

        RG::RHI_TextureHandle depth = fg.CreateTexture({
            width,
            height,
            RG::RHI_TextureFormat::Depth32,
            RG::RHI_TextureUsage::DepthAttachment | RG::RHI_TextureUsage::Sampled,
        });

        RG::RHI_TextureHandle shadowMaps = fg.CreateTexture({
            Nova::Core::Renderer::RHI::SHADOW_MAP_RESOLUTION,
            Nova::Core::Renderer::RHI::SHADOW_MAP_RESOLUTION,
            RG::RHI_TextureFormat::Depth32,
            RG::RHI_TextureUsage::DepthAttachment | RG::RHI_TextureUsage::Sampled,
            Nova::Core::Renderer::RHI::MAX_SHADOW_MAPS,
            /*m_ResizeWithViewport*/ false,
            /*m_ComparisonSampler*/ true,
        });

        const RG::RHI_TextureUsage selectionRtUsage = RG::RHI_TextureUsage::ColorAttachment | RG::RHI_TextureUsage::Sampled;

        RG::RHI_TextureHandle selectionMask = fg.CreateTexture({
            width, height, RG::RHI_TextureFormat::RGBA8, selectionRtUsage,
            /*m_Layers*/ 1,
            /*m_ResizeWithViewport*/ true,
            /*m_ComparisonSampler*/ false,
            /*m_RegisterImGui*/ false,
        });
        RG::RHI_TextureHandle selectionBlurTemp = fg.CreateTexture({
            width, height, RG::RHI_TextureFormat::RGBA8, selectionRtUsage,
            /*m_Layers*/ 1,
            /*m_ResizeWithViewport*/ true,
            /*m_ComparisonSampler*/ false,
            /*m_RegisterImGui*/ false,
        });
        RG::RHI_TextureHandle selectionBlurred = fg.CreateTexture({
            width, height, RG::RHI_TextureFormat::RGBA8, selectionRtUsage,
            /*m_Layers*/ 1,
            /*m_ResizeWithViewport*/ true,
            /*m_ComparisonSampler*/ false,
            /*m_RegisterImGui*/ false,
        });

        m_SceneColor = color;
        m_SceneDepth = depth;
        m_ShadowMaps = shadowMaps;
        m_SelectionMask = selectionMask;
        m_SelectionBlurTemp = selectionBlurTemp;
        m_SelectionBlurred = selectionBlurred;

        auto acquireShader = [](const std::filesystem::path& uri) {
            auto handle = AssetManager::Get().Acquire<ShaderAsset>(uri);
            if (!AssetManager::Get().Load(handle)) {
                NV_LOG_WARN(("Failed to compile shader asset: " + uri.generic_string()).c_str());
                if (auto asset = AssetManager::Get().GetAsset(handle); asset && !asset->GetLastLog().empty())
                    NV_LOG_WARN(asset->GetLastLog().c_str());
            }
            return AssetManager::Get().GetAsset(handle);
        };

        auto gridVert = acquireShader("Editor://Shaders/Grid.vert.slang");
        auto gridFrag = acquireShader("Editor://Shaders/Grid.frag.slang");
        auto sceneVert = acquireShader("Engine://Shaders/Scene.vert.slang");
        auto sceneFrag = acquireShader("Engine://Shaders/Scene.frag.slang");
        auto unlitFrag = acquireShader("Engine://Shaders/Unlit.frag.slang");
        auto shadowVert = acquireShader("Engine://Shaders/Shadow.vert.slang");
        auto normalsFrag = acquireShader("Engine://Shaders/NormalsDebug.frag.slang");
        auto positionsFrag = acquireShader("Engine://Shaders/PositionsDebug.frag.slang");
        auto vertexColorFrag = acquireShader("Engine://Shaders/VertexColor.frag.slang");
        auto depthFrag = acquireShader("Engine://Shaders/DepthDebug.frag.slang");
        auto selectionFullscreenVert = acquireShader("Editor://Shaders/SelectionFullscreen.vert.slang");
        auto selectionMaskVert = acquireShader("Editor://Shaders/SelectionMask.vert.slang");
        auto selectionMaskFrag = acquireShader("Editor://Shaders/SelectionMask.frag.slang");
        auto selectionMaskOccludedFrag = acquireShader("Editor://Shaders/SelectionMaskOccluded.frag.slang");
        auto selectionBlurHFrag = acquireShader("Editor://Shaders/SelectionOutlineBlurH.frag.slang");
        auto selectionBlurVFrag = acquireShader("Editor://Shaders/SelectionOutlineBlurV.frag.slang");
        auto selectionCompositeFrag = acquireShader("Editor://Shaders/SelectionOutlineComposite.frag.slang");

        m_GridShader = fg.RegisterShader({
            .m_Name = "Grid",
            .m_Vertex = gridVert,
            .m_Fragment = gridFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::FullscreenQuad,
            .m_AlphaBlend = true,
        });

        m_SceneShader = fg.RegisterShader({
            .m_Name = "Scene",
            .m_Vertex = sceneVert,
            .m_Fragment = sceneFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::Mesh,
        });
        m_UnlitShader = fg.RegisterShader({
            .m_Name = "Unlit",
            .m_Vertex = sceneVert,
            .m_Fragment = unlitFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::Mesh,
        });
        m_WireframeShader = fg.RegisterShader({
            .m_Name = "SceneWireframe",
            .m_Vertex = sceneVert,
            .m_Fragment = sceneFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::Mesh,
            .m_CullMode = RG::RHI_CullMode::None,
            .m_PolygonMode = RG::RHI_PolygonMode::Line,
        });
        m_ShadowShader = fg.RegisterShader({
            .m_Name = "Shadow",
            .m_Vertex = shadowVert,
            .m_Fragment = nullptr,
            .m_VertexLayout = RG::RHI_VertexLayout::Mesh,
            .m_AlphaBlend = false,
            .m_DepthTest = true,
            .m_DepthWrite = true,
            .m_DepthOnly = true,
            .m_CullMode = RG::RHI_CullMode::Front,
            .m_DepthBiasConstant = 0.5f,
            .m_DepthBiasSlope = 1.0f,
        });
        m_NormalsShader = fg.RegisterShader({
            .m_Name = "NormalsDebug",
            .m_Vertex = sceneVert,
            .m_Fragment = normalsFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::Mesh,
        });
        m_PositionsShader = fg.RegisterShader({
            .m_Name = "PositionsDebug",
            .m_Vertex = sceneVert,
            .m_Fragment = positionsFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::Mesh,
        });
        m_VertexColorShader = fg.RegisterShader({
            .m_Name = "VertexColor",
            .m_Vertex = sceneVert,
            .m_Fragment = vertexColorFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::Mesh,
        });
        m_DepthShader = fg.RegisterShader({
            .m_Name = "DepthDebug",
            .m_Vertex = sceneVert,
            .m_Fragment = depthFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::Mesh,
        });
        m_AABBShader = fg.RegisterShader({
            .m_Name = "AABBDebug",
            .m_Vertex = sceneVert,
            .m_Fragment = vertexColorFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::Mesh,
            .m_PrimitiveTopology = RG::RHI_PrimitiveTopology::Lines,
            .m_DepthTest = false,
            .m_DepthWrite = false,
            .m_CullMode = RG::RHI_CullMode::None,
        });
        m_SelectionMaskShader = fg.RegisterShader({
            .m_Name = "SelectionMask",
            .m_Vertex = selectionMaskVert,
            .m_Fragment = selectionMaskFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::Mesh,
            .m_DepthTest = true,
            .m_DepthWrite = false,
            .m_CullMode = RG::RHI_CullMode::Back,
            .m_DepthCompare = RG::RHI_DepthCompare::LessOrEqual,
        });
        m_SelectionMaskOccludedShader = fg.RegisterShader({
            .m_Name = "SelectionMaskOccluded",
            .m_Vertex = selectionMaskVert,
            .m_Fragment = selectionMaskOccludedFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::Mesh,
            .m_DepthTest = true,
            .m_DepthWrite = false,
            .m_CullMode = RG::RHI_CullMode::Back,
            .m_DepthCompare = RG::RHI_DepthCompare::Greater,
        });
        m_SelectionBlurHShader = fg.RegisterShader({
            .m_Name = "SelectionBlurH",
            .m_Vertex = selectionFullscreenVert,
            .m_Fragment = selectionBlurHFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::FullscreenQuad,
            .m_DepthTest = false,
            .m_DepthWrite = false,
            .m_CullMode = RG::RHI_CullMode::None,
        });
        m_SelectionBlurVShader = fg.RegisterShader({
            .m_Name = "SelectionBlurV",
            .m_Vertex = selectionFullscreenVert,
            .m_Fragment = selectionBlurVFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::FullscreenQuad,
            .m_DepthTest = false,
            .m_DepthWrite = false,
            .m_CullMode = RG::RHI_CullMode::None,
        });
        m_SelectionCompositeShader = fg.RegisterShader({
            .m_Name = "SelectionComposite",
            .m_Vertex = selectionFullscreenVert,
            .m_Fragment = selectionCompositeFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::FullscreenQuad,
            .m_AlphaBlend = true,
            .m_AdditiveBlend = true,
            .m_DepthTest = false,
            .m_DepthWrite = false,
            .m_CullMode = RG::RHI_CullMode::None,
        });

        fg.AddPass("Shadow",
            [&](RG::RHI_PassBuilder& b) {
                b.Write(shadowMaps);
            },
            [this](RG::IPassContext& ctx) {
                RenderShadowPass(ctx);
            });

        fg.AddPass("Grid",
            [&](RG::RHI_PassBuilder& b) {
                b.Write(color);
                if (depth.IsValid())
                    b.Write(depth);
            },
            [this](RG::IPassContext& ctx) {
                if (!m_ShowGrid)
                    return;
                auto* shader = ctx.GetShader(m_GridShader);
                if (!shader) return;
                const glm::vec3 resolution(ctx.GetRenderWidth(), ctx.GetRenderHeight(), 1.0f);
                shader->SetParameter("m_Resolution", resolution);
                ctx.DrawFullscreen(m_GridShader);
            });

        fg.AddPass("Scene",
            [&](RG::RHI_PassBuilder& b) {
                b.Read(color);
                b.Write(color);
                if (depth.IsValid()) {
                    b.Read(depth);
                    b.Write(depth);
                }
                b.Read(shadowMaps);
            },
            [this](RG::IPassContext& ctx) {
                RenderScene(ctx);
                if (m_ShowAABB)
                    RenderAABBs(ctx);
            });

        fg.AddPass("SelectionMask",
            [&](RG::RHI_PassBuilder& b) {
                b.Write(selectionMask);
                if (depth.IsValid()) {
                    b.Read(depth);
                    b.Write(depth);
                }
            },
            [this](RG::IPassContext& ctx) {
                RenderSelectionMask(ctx);
            });

        fg.AddPass("SelectionBlurH",
            [&](RG::RHI_PassBuilder& b) {
                b.Read(selectionMask);
                b.Write(selectionBlurTemp);
                if (depth.IsValid()) {
                    b.Read(depth);
                    b.Write(depth);
                }
            },
            [this](RG::IPassContext& ctx) {
                RenderSelectionBlur(ctx, m_SelectionBlurHShader);
            });

        fg.AddPass("SelectionBlurV",
            [&](RG::RHI_PassBuilder& b) {
                b.Read(selectionBlurTemp);
                b.Write(selectionBlurred);
                if (depth.IsValid()) {
                    b.Read(depth);
                    b.Write(depth);
                }
            },
            [this](RG::IPassContext& ctx) {
                RenderSelectionBlur(ctx, m_SelectionBlurVShader);
            });

        fg.AddPass("SelectionComposite",
            [&](RG::RHI_PassBuilder& b) {
                b.Read(selectionMask);
                b.Read(selectionBlurred);
                b.Read(color);
                b.Write(color);
                if (depth.IsValid()) {
                    b.Read(depth);
                    b.Write(depth);
                }
            },
            [this](RG::IPassContext& ctx) {
                RenderSelectionComposite(ctx);
            });

        fg.AddPass("UI",
            [&](RG::RHI_PassBuilder& b) {
                b.Write(backbuffer);
                b.PresentOnly();
            },
            [](RG::IPassContext& /*ctx*/) {
            });

        m_Renderer->SetRenderGraph(fg.Build(api));

        if (auto* graph = m_Renderer->GetRenderGraph()) {
            graph->BindEngineShadowMaps(m_ShadowMaps);
            BindSelectionOutlineTextures();
        }

        m_AABBWireframeMesh = Nova::Core::Math::CreateUnitAABBWireframeMesh(glm::vec3(0.0f, 1.0f, 0.0f));
    }

    void AppRenderer::Shutdown() {
        if (m_Renderer) {
            m_Renderer->Destroy();
            m_Renderer.reset();
        }
    }

    void AppRenderer::BeginFrame() {
        NV_ASSERT_MSG(m_Renderer, "Renderer is not initialized.");
        m_Renderer->BeginFrame();
    }

    void AppRenderer::RenderFrame() {
        NV_ASSERT_MSG(m_Renderer, "Renderer is not initialized.");
        m_Renderer->RenderFrame();
    }

    void AppRenderer::EndFrame() {
        NV_ASSERT_MSG(m_Renderer, "Renderer is not initialized.");
        m_Renderer->EndFrame();
    }

    void AppRenderer::Resize(int width, int height) {
        NV_ASSERT_MSG(m_Renderer, "Renderer is not initialized.");
        m_Renderer->Resize(width, height);
    }

    void AppRenderer::BindFrame(
        Nova::Core::Scene::Scene& scene,
        Nova::Core::Math::Camera& camera,
        Editor::EditorSelection* selection)
    {
        m_Scene = &scene;
        m_Camera = &camera;
        m_Selection = selection;
    }

    void AppRenderer::RebindAfterResize() {
        if (!m_Renderer)
            return;
        if (auto* graph = m_Renderer->GetRenderGraph()) {
            graph->BindEngineShadowMaps(m_ShadowMaps);
            BindSelectionOutlineTextures();
        }
    }

    Nova::Core::Renderer::RHI::RHI_ShaderHandle AppRenderer::GetActiveSceneShader() const {
        switch (m_RenderDebugMode) {
            case RenderDebugMode::Unlit:       return m_UnlitShader;
            case RenderDebugMode::Wireframe:   return m_WireframeShader;
            case RenderDebugMode::Normals:     return m_NormalsShader;
            case RenderDebugMode::Positions:   return m_PositionsShader;
            case RenderDebugMode::VertexColor: return m_VertexColorShader;
            case RenderDebugMode::Depth:       return m_DepthShader;
            case RenderDebugMode::Lit:
            default:
                return m_SceneShader;
        }
    }

    void AppRenderer::PushFrameUniforms(
        float elapsedTime,
        float deltaTime,
        uint32_t& frameIndex,
        const glm::vec2& viewportSize)
    {
        NV_ASSERT_MSG(m_Renderer, "Renderer is not initialized.");

        auto setFrameUniforms = [elapsedTime, deltaTime, &frameIndex, viewportSize](
            Nova::Core::Renderer::RHI::IShaders* shader)
        {
            if (!shader) return;
            shader->SetParameter("m_Time", elapsedTime);
            shader->SetParameter("m_TimeDelta", deltaTime);
            shader->SetParameter("m_FrameRate", deltaTime > 0.0f ? 1.0f / deltaTime : 0.0f);
            shader->SetParameter("m_Frame", static_cast<int>(frameIndex++));
            shader->SetParameter("m_Resolution", glm::vec3(viewportSize.x, viewportSize.y, 1.0f));
        };

        if (auto* graph = m_Renderer->GetRenderGraph()) {
            setFrameUniforms(graph->GetShader(m_GridShader));
            setFrameUniforms(graph->GetShader(m_SceneShader));
            setFrameUniforms(graph->GetShader(m_UnlitShader));
            setFrameUniforms(graph->GetShader(m_WireframeShader));
            setFrameUniforms(graph->GetShader(m_ShadowShader));
            setFrameUniforms(graph->GetShader(m_NormalsShader));
            setFrameUniforms(graph->GetShader(m_PositionsShader));
            setFrameUniforms(graph->GetShader(m_VertexColorShader));
            setFrameUniforms(graph->GetShader(m_DepthShader));
            setFrameUniforms(graph->GetShader(m_AABBShader));
            setFrameUniforms(graph->GetShader(m_SelectionMaskShader));
            setFrameUniforms(graph->GetShader(m_SelectionMaskOccludedShader));
        }
    }

    const Nova::Core::Renderer::RHI::RHI_EngineParameterBlock* AppRenderer::GetEngine() const {
        if (!m_Renderer)
            return nullptr;
        auto* graph = m_Renderer->GetRenderGraph();
        return graph ? graph->GetEngineParameterBlock() : nullptr;
    }

    void AppRenderer::UploadSceneUniformsWithLightIndex(int lightIndex) {
        NV_ASSERT_MSG(m_Renderer, "Renderer is not initialized.");
        NV_ASSERT_MSG(m_Camera, "Camera is not bound for this frame.");

        const auto* engine = GetEngine();
        if (!engine || !engine->m_Scene.m_Uniforms.IsValid())
            return;

        Nova::Core::Renderer::RHI::SceneUniforms scene{};
        scene.m_Camera.m_View = m_Camera->GetViewMatrix();
        scene.m_Camera.m_Proj = m_Camera->GetProjectionMatrix();
        scene.m_Camera.m_ViewProj = scene.m_Camera.m_Proj * scene.m_Camera.m_View;
        scene.m_Camera.m_InvViewProj = glm::inverse(scene.m_Camera.m_ViewProj);
        scene.m_Camera.m_Position = m_Camera->m_LookFrom;
        scene.m_LightCount = static_cast<int>(m_GpuLights.size());
        scene.m_LightIndex = lightIndex;

        m_Renderer->UpdateGpuBuffer(
            engine->m_Scene.m_Uniforms,
            &scene,
            sizeof(scene),
            0);
    }

    void AppRenderer::PushSceneUniforms() {
        UploadSceneUniformsWithLightIndex(0);
    }

    void AppRenderer::PrepareGpuInstances() {
        NV_ASSERT_MSG(m_Renderer, "Renderer is not initialized.");
        NV_ASSERT_MSG(m_Scene, "Scene is not bound for this frame.");

        const auto* engine = GetEngine();
        if (!engine || !engine->m_Instances.IsValid() || !engine->m_Materials.IsValid())
            return;

        m_SceneBatches = Nova::Core::Scene::BuildRenderBatches(*m_Scene, 0);

        m_AABBInstances.clear();
        m_AABBBatches.clear();
        m_AABBFirstInstance = static_cast<uint32_t>(m_SceneBatches.m_Instances.size());
        if (m_ShowAABB && m_AABBWireframeMesh) {
            auto& registry = m_Scene->GetRegistry();
            auto view = registry.view<TransformComponent, MeshComponent>();
            for (auto entity : view) {
                auto& mc = view.get<MeshComponent>(entity);
                if (!mc.m_AABBTree.IsBuilt())
                    continue;

                const glm::mat4 entityTransform = m_Scene->GetWorldTransform(entity);
                for (const auto& node : mc.m_AABBTree.GetNodes()) {
                    Nova::Core::Renderer::RHI::InstanceGPU& instance = m_AABBInstances.emplace_back();
                    instance.m_Model = entityTransform * Nova::Core::Math::AABBToModelMatrix(node.m_Bounds);
                    instance.m_MaterialIndex = 0;
                }
            }
            if (!m_AABBInstances.empty()) {
                m_AABBBatches.push_back({
                    Nova::Core::kInvalidUUID,
                    m_AABBWireframeMesh,
                    m_AABBFirstInstance,
                    static_cast<uint32_t>(m_AABBInstances.size()),
                });
            }
        }

        m_SelectionInstances.clear();
        m_SelectionBatches.clear();
        m_SelectionFirstInstance = m_AABBFirstInstance + static_cast<uint32_t>(m_AABBInstances.size());
        if (m_Selection && !m_Selection->Empty()) {
            auto& registry = m_Scene->GetRegistry();
            auto& selected = m_Selection->GetEntities();
            std::erase_if(selected, [&](entt::entity entity) { return !registry.valid(entity); });

            // One instance + one draw per selected entity (keeps mesh pairing trivial).
            for (entt::entity entity : selected) {
                auto* mrc = registry.try_get<MeshRendererComponent>(entity);
                auto meshAsset = mrc ? AssetManager::Get().GetAsset(mrc->m_Mesh) : nullptr;
                if (!registry.all_of<TransformComponent>(entity) || !mrc || !meshAsset || !meshAsset->IsLoaded())
                    continue;
                auto cpuMesh = meshAsset->GetCPUMesh();
                if (!cpuMesh)
                    continue;

                const uint32_t first = m_SelectionFirstInstance + static_cast<uint32_t>(m_SelectionInstances.size());
                Nova::Core::Renderer::RHI::InstanceGPU& instance = m_SelectionInstances.emplace_back();
                instance.m_Model = m_Scene->GetWorldTransform(entity);
                instance.m_MaterialIndex = 0;
                m_SelectionBatches.push_back({
                    mrc->m_Mesh.GetUUID(),
                    cpuMesh,
                    first,
                    1u,
                });
            }
        }

        const uint32_t totalInstances = static_cast<uint32_t>(
            m_SceneBatches.m_Instances.size() + m_AABBInstances.size() + m_SelectionInstances.size());
        Nova::Core::Renderer::RHI::EnsureGpuBufferCapacity(
            *m_Renderer, engine->m_Instances, std::max(totalInstances, 1u));
        Nova::Core::Renderer::RHI::EnsureGpuBufferCapacity(
            *m_Renderer, engine->m_Materials, std::max(static_cast<uint32_t>(m_SceneBatches.m_Materials.size()), 1u));

        if (!m_SceneBatches.m_Instances.empty()) {
            Nova::Core::Renderer::RHI::UpdateStructuredBufferRange<Nova::Core::Renderer::RHI::InstanceGPU>(
                *m_Renderer, engine->m_Instances,
                std::span<const Nova::Core::Renderer::RHI::InstanceGPU>(m_SceneBatches.m_Instances), 0);
        }
        if (!m_AABBInstances.empty()) {
            Nova::Core::Renderer::RHI::UpdateStructuredBufferRange<Nova::Core::Renderer::RHI::InstanceGPU>(
                *m_Renderer, engine->m_Instances,
                std::span<const Nova::Core::Renderer::RHI::InstanceGPU>(m_AABBInstances), m_AABBFirstInstance);
        }
        if (!m_SelectionInstances.empty()) {
            Nova::Core::Renderer::RHI::UpdateStructuredBufferRange<Nova::Core::Renderer::RHI::InstanceGPU>(
                *m_Renderer, engine->m_Instances,
                std::span<const Nova::Core::Renderer::RHI::InstanceGPU>(m_SelectionInstances), m_SelectionFirstInstance);
        }
        if (!m_SceneBatches.m_Materials.empty()) {
            Nova::Core::Renderer::RHI::UpdateStructuredBufferRange<Nova::Core::Renderer::RHI::MaterialGPU>(
                *m_Renderer, engine->m_Materials,
                std::span<const Nova::Core::Renderer::RHI::MaterialGPU>(m_SceneBatches.m_Materials), 0);
        }
    }

    void AppRenderer::DrawInstanceBatches(
        Nova::Core::Renderer::RHI::IPassContext& ctx,
        Nova::Core::Renderer::RHI::RHI_ShaderHandle shader,
        const std::vector<Nova::Core::Scene::SceneInstanceBatch>& batches,
        Nova::Core::Renderer::RHI::RHI_PrimitiveTopology topology)
    {
        if (!shader.IsValid() || batches.empty())
            return;

        ctx.BindShader(shader);
        for (const auto& batch : batches) {
            if (!batch.m_Mesh || batch.m_InstanceCount == 0)
                continue;

            Nova::Core::Renderer::RHI::RHI_DrawIndexedCommand cmd{};
            cmd.m_Mesh = batch.m_Mesh;
            cmd.m_Topology = topology;
            cmd.m_IndexType = Nova::Core::Renderer::RHI::RHI_IndexType::UInt32;
            cmd.m_IndexCount = batch.m_Mesh->GetIndexCount();
            cmd.m_InstanceCount = batch.m_InstanceCount;
            cmd.m_FirstInstance = batch.m_FirstInstance;
            ctx.DrawIndexed(cmd);
        }
    }

    void AppRenderer::RenderScene(Nova::Core::Renderer::RHI::IPassContext& ctx) {
        NV_ASSERT_MSG(m_Renderer, "Renderer is not initialized.");
        NV_ASSERT_MSG(m_Camera, "Camera is not bound for this frame.");
        NV_ASSERT_MSG(m_Scene, "Scene is not bound for this frame.");

        DrawInstanceBatches(
            ctx,
            GetActiveSceneShader(),
            m_SceneBatches.m_Batches,
            Nova::Core::Renderer::RHI::RHI_PrimitiveTopology::Triangles);
    }

    void AppRenderer::RenderSelectionMask(Nova::Core::Renderer::RHI::IPassContext& ctx) {
        if (!m_Selection || m_Selection->Empty() || m_SelectionBatches.empty())
            return;

        // Occluded (R+G) then visible (R, G→0): both write R so the blur halo
        // uses a full silhouette; visible clears G on the front surface so
        // self-occlusion does not leave a yellow x-ray fill.
        DrawInstanceBatches(
            ctx,
            m_SelectionMaskOccludedShader,
            m_SelectionBatches,
            Nova::Core::Renderer::RHI::RHI_PrimitiveTopology::Triangles);
        DrawInstanceBatches(
            ctx,
            m_SelectionMaskShader,
            m_SelectionBatches,
            Nova::Core::Renderer::RHI::RHI_PrimitiveTopology::Triangles);
    }

    void AppRenderer::RenderSelectionBlur(
        Nova::Core::Renderer::RHI::IPassContext& ctx,
        Nova::Core::Renderer::RHI::RHI_ShaderHandle blurShader)
    {
        if (!m_Selection || m_Selection->Empty() || !blurShader.IsValid())
            return;
        ctx.DrawFullscreen(blurShader);
    }

    void AppRenderer::RenderSelectionComposite(Nova::Core::Renderer::RHI::IPassContext& ctx) {
        if (!m_Selection || m_Selection->Empty() || !m_SelectionCompositeShader.IsValid())
            return;
        ctx.DrawFullscreen(m_SelectionCompositeShader);
    }

    void AppRenderer::BindSelectionOutlineTextures() {
        auto* graph = m_Renderer ? m_Renderer->GetRenderGraph() : nullptr;
        if (!graph)
            return;

        auto bindOutlineTexture = [&](
            Nova::Core::Renderer::RHI::RHI_ShaderHandle shaderHandle,
            const char* textureName,
            const char* samplerName,
            Nova::Core::Renderer::RHI::RHI_TextureHandle textureHandle)
        {
            auto* shader = graph->GetShader(shaderHandle);
            if (!shader)
                return;
            uint64_t imageView = 0;
            uint64_t sampler = 0;
            if (!graph->GetSampledTextureNativeHandles(textureHandle, imageView, sampler))
                return;
            shader->BindSampledTexture(textureName, samplerName, imageView, sampler);
        };

        bindOutlineTexture(
            m_SelectionBlurHShader, "outline.source", "outline.sourceSampler", m_SelectionMask);
        bindOutlineTexture(
            m_SelectionBlurVShader, "outline.source", "outline.sourceSampler", m_SelectionBlurTemp);
        bindOutlineTexture(
            m_SelectionCompositeShader,
            "outline.selectionMask", "outline.selectionMaskSampler",
            m_SelectionMask);
        bindOutlineTexture(
            m_SelectionCompositeShader,
            "outline.selectionBlurred", "outline.selectionBlurredSampler",
            m_SelectionBlurred);
    }

    void AppRenderer::RenderAABBs(Nova::Core::Renderer::RHI::IPassContext& ctx) {
        if (!m_AABBWireframeMesh || m_AABBBatches.empty())
            return;

        DrawInstanceBatches(
            ctx,
            m_AABBShader,
            m_AABBBatches,
            Nova::Core::Renderer::RHI::RHI_PrimitiveTopology::Lines);
    }

    void AppRenderer::UploadLights() {
        NV_ASSERT_MSG(m_Scene, "Scene is not bound for this frame.");

        m_GpuLights.clear();
        auto& registry = m_Scene->GetRegistry();
        auto view = registry.view<TransformComponent, LightComponent>();

        int nextShadow = 0;
        for (auto entity : view) {
            if (m_GpuLights.size() >= Nova::Core::Renderer::RHI::MAX_LIGHTS)
                break;

            auto& lc = view.get<LightComponent>(entity);
            if (!lc.m_Light)
                continue;

            const Light& light = *lc.m_Light;
            const glm::mat4 world = m_Scene->GetWorldTransform(entity);
            const glm::vec3 position = glm::vec3(world[3]);
            // Directional / Spot: beam from world rotation (local +X).
            glm::vec3 travelDir{ 0.0f };
            if (light.m_Type == LightType::Directional || light.m_Type == LightType::Spot) {
                travelDir = glm::mat3(world) * LightLocalBeamAxis();
                const float len2 = glm::dot(travelDir, travelDir);
                travelDir = len2 > 1e-12f ? (travelDir / std::sqrt(len2)) : LightLocalBeamAxis();
            }


            Nova::Core::Renderer::RHI::LightGPU gpu{};
            gpu.m_Type = static_cast<int>(light.m_Type);
            gpu.m_Intensity = light.m_Intensity;
            gpu.m_Color = light.m_Color;
            gpu.m_Range = light.m_Range;
            gpu.m_Direction = travelDir;
            gpu.m_InnerConeCos = light.InnerCos();
            gpu.m_Position = position;
            gpu.m_OuterConeCos = light.OuterCos();
            gpu.m_ShadowBiasConstant = light.m_ShadowBiasConstant;
            gpu.m_ShadowBiasSlope = light.m_ShadowBiasSlope;
            gpu.m_ShadowNormalBias = light.m_ShadowNormalBias;
            gpu.m_CastShadow = 0;
            gpu.m_ShadowMapIndex = -1;

            const bool canShadow = light.m_LightShadow
                && (light.m_Type == LightType::Directional || light.m_Type == LightType::Spot)
                && nextShadow < static_cast<int>(Nova::Core::Renderer::RHI::MAX_SHADOW_MAPS);
            if (canShadow) {
                gpu.m_CastShadow = 1;
                gpu.m_ShadowMapIndex = nextShadow++;
                gpu.m_LightViewProj = BuildLightViewProj(
                    light.m_Type, position, travelDir, light.m_Range, light.m_OuterCone,
                    Nova::Core::Renderer::RHI::SHADOW_DIR_ORTHO_HALF_EXTENT);
            }

            m_GpuLights.push_back(gpu);
        }

        auto* graph = m_Renderer ? m_Renderer->GetRenderGraph() : nullptr;
        const auto* engine = graph ? graph->GetEngineParameterBlock() : nullptr;
        if (!engine || !engine->m_Scene.m_Lights.IsValid())
            return;

        if (!m_GpuLights.empty()) {
            m_Renderer->UpdateGpuBuffer(
                engine->m_Scene.m_Lights,
                m_GpuLights.data(),
                sizeof(Nova::Core::Renderer::RHI::LightGPU) * m_GpuLights.size(),
                0);
        }
    }

    void AppRenderer::RenderShadowPass(Nova::Core::Renderer::RHI::IPassContext& ctx) {
        if (!m_Renderer || !m_ShadowMaps.IsValid() || !m_Scene)
            return;

        for (uint32_t layer = 0; layer < Nova::Core::Renderer::RHI::MAX_SHADOW_MAPS; ++layer) {
            ctx.BeginDepthLayer(m_ShadowMaps, layer, true);
            ctx.EndDepthLayer();
        }

        if (!ctx.GetShader(m_ShadowShader) || m_SceneBatches.m_Batches.empty())
            return;

        for (size_t lightIdx = 0; lightIdx < m_GpuLights.size(); ++lightIdx) {
            const auto& light = m_GpuLights[lightIdx];
            if (!light.m_CastShadow || light.m_ShadowMapIndex < 0)
                continue;

            ctx.BeginDepthLayer(m_ShadowMaps, static_cast<uint32_t>(light.m_ShadowMapIndex), true);
            const float angleFactor = ShadowAngleBiasFactor(light.m_Direction);
            const float typeScale = (light.m_Type == static_cast<int>(LightType::Spot)) ? 0.35f : 1.0f;
            ctx.SetDepthBias(
                light.m_ShadowBiasConstant * angleFactor * typeScale,
                light.m_ShadowBiasSlope * angleFactor * typeScale);

            UploadSceneUniformsWithLightIndex(static_cast<int>(lightIdx));
            DrawInstanceBatches(
                ctx,
                m_ShadowShader,
                m_SceneBatches.m_Batches,
                Nova::Core::Renderer::RHI::RHI_PrimitiveTopology::Triangles);

            ctx.EndDepthLayer();
        }

        // Restore scene light index after shadow recording.
        UploadSceneUniformsWithLightIndex(0);
    }

} // namespace Nova::App