#include "AssimpLayer.h"

#include <filesystem>
#include <string>

#include "imgui.h"

#include "Asset/AssetManager.h"
#include "Asset/Assets/ShaderAsset.h"
#include "Core/Application.h"
#include "Core/GraphicsAPI.h"
#include "Core/Log.h"
#include "Core/UUID.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/MeshRendererComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "Events/ApplicationEvents.h"
#include "Renderer/RHI/RHI_RenderGraphBuilder.h"
#include "Renderer/RHI/RHI_ShaderUniforms.h"
#include "Renderer/RHI/RHI_Shaders.h"

constexpr const char* kSuzannePath =
    "Nova-Tests/AssimpObjectLoading/Resources/Assets/Meshes/suzanne.obj";
constexpr const char* kSimpleLitVertPath =
    "Nova-Tests/AssimpObjectLoading/Resources/Assets/Shaders/SimpleLit.vert.slang";
constexpr const char* kSimpleLitFragPath =
    "Nova-Tests/AssimpObjectLoading/Resources/Assets/Shaders/SimpleLit.frag.slang";

AssimpLayer::AssimpLayer() : Layer("AssimpLayer") {}

void AssimpLayer::OnAttach() {
    NV_LOG_INFO("AssimpLayer attached");

    auto& app = Nova::Core::Application::Get();
    auto& window = app.GetWindow();
    auto& imgui = app.GetImGuiLayer();

    const auto api = Nova::Core::GraphicsAPI::Vulkan;

    if (window.GetGraphicsAPI() == Nova::Core::GraphicsAPI::SDLRenderer)
        imgui.DestroyImGuiBackend(Nova::Core::GraphicsAPI::SDLRenderer);

    if (window.GetGraphicsAPI() != api) {
        if (!window.SetGraphicsAPI(api)) {
            NV_LOG_ERROR("AssimpLayer: failed to set GraphicsAPI::Vulkan");
            return;
        }
    }

    imgui.SetImGuiBackend(api);

    int width = 0, height = 0;
    window.GetWindowSize(width, height);
    if (width <= 0 || height <= 0) {
        width = 1280;
        height = 720;
    }
    m_ViewportSize = { static_cast<float>(width), static_cast<float>(height) };

    SetupRenderer(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    SetupScene();
}

void AssimpLayer::SetupRenderer(uint32_t width, uint32_t height) {
    namespace RHI = Nova::Core::Renderer::RHI;
    using namespace Nova::Core::Asset;
    using namespace Nova::Core::Asset::Assets;

    RHI::RHI_SwapchainDesc swapDesc{};
    swapDesc.m_FramesInFlight = 3;
    swapDesc.m_CreateSurface = true;
    swapDesc.m_EnableSwapchain = true;
    swapDesc.m_PreferredPresentMode = RHI::RHI_PresentMode::Default;

    m_Renderer = RHI::IRenderer::Create(Nova::Core::GraphicsAPI::Vulkan, swapDesc);
    if (!m_Renderer) {
        NV_LOG_ERROR("AssimpLayer: failed to create RHI renderer");
        return;
    }

    auto acquireShader = [](const std::filesystem::path& uri) {
        auto asset = AssetManager::Get().Acquire<ShaderAsset>(uri).GetAssetRef();
        if (!asset || !asset->Compile()) {
            NV_LOG_WARN(("Failed to compile shader: " + uri.generic_string()).c_str());
            if (asset && !asset->GetLastLog().empty())
                NV_LOG_WARN(asset->GetLastLog().c_str());
        }
        return asset;
    };

    auto litVert = acquireShader(kSimpleLitVertPath);
    auto litFrag = acquireShader(kSimpleLitFragPath);

    RHI::RHI_RenderGraphBuilder rg;

    const auto backbuffer = rg.ImportTexture({
        width, height,
        RHI::RHI_TextureFormat::RGBA8,
        RHI::RHI_TextureUsage::ColorAttachment,
    }, RHI::RHI_ResourceState::Present);

    m_ShadowMaps = rg.CreateTexture({
        RHI::SHADOW_MAP_RESOLUTION,
        RHI::SHADOW_MAP_RESOLUTION,
        RHI::RHI_TextureFormat::Depth32,
        RHI::RHI_TextureUsage::DepthAttachment | RHI::RHI_TextureUsage::Sampled,
        RHI::MAX_SHADOW_MAPS,
        /*m_ResizeWithViewport*/ false,
        /*m_ComparisonSampler*/ true,
    });

    m_SimpleLitShader = rg.RegisterShader({
        .m_Name = "SimpleLit",
        .m_Vertex = litVert,
        .m_Fragment = litFrag,
        .m_VertexLayout = RHI::RHI_VertexLayout::Mesh,
    });

    rg.AddPass("Scene",
        [&](RHI::RHI_PassBuilder& b) {
            b.Write(backbuffer);
            b.Read(m_ShadowMaps);
        },
        [this](RHI::IPassContext& ctx) {
            RenderScene(ctx);
        });

    rg.AddPass("UI",
        [&](RHI::RHI_PassBuilder& b) {
            b.Write(backbuffer);
            b.PresentOnly();
        },
        [](RHI::IPassContext& /*ctx*/) {});

    m_Renderer->SetRenderGraph(rg.Build(Nova::Core::GraphicsAPI::Vulkan));

    if (auto* graph = m_Renderer->GetRenderGraph())
        graph->BindEngineShadowMaps(m_ShadowMaps);

    NV_LOG_INFO("AssimpLayer: renderer ready (SimpleLit)");
}

void AssimpLayer::SetupScene() {
    using namespace Nova::Core::Asset;
    using namespace Nova::Core::Asset::Assets;
    using namespace Nova::Core::ECS::Components;

    m_Camera = std::make_shared<Nova::Core::Math::Camera>(
        glm::vec3(3.5f, 2.0f, 4.0f),
        glm::vec3(0.0f, 0.35f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        45.0f,
        m_ViewportSize.x / m_ViewportSize.y,
        0.1f,
        100.0f,
        true);

    auto& registry = m_Scene.GetRegistry();

    // Engine primitive (stable AssetID = 1).
    m_CubeAsset = AssetManager::Get()
        .Acquire<MeshAsset>("Engine://Primitives/Cube")
        .GetAssetRef();
    m_CubeLoaded = m_CubeAsset && m_CubeAsset->Load();
    if (m_CubeLoaded) {
        m_CubeEntity = m_Scene.CreateEntity("Cube");
        registry.emplace<TransformComponent>(
            m_CubeEntity,
            glm::vec3(-1.4f, 0.5f, 0.0f),
            glm::vec3(0.0f),
            glm::vec3(1.0f));

        Nova::Core::Renderer::RHI::MaterialGPU mat{};
        mat.m_BaseColor = glm::vec3(0.35f, 0.55f, 0.95f);
        registry.emplace<MeshRendererComponent>(m_CubeEntity, m_CubeAsset, mat);
        registry.emplace<MeshComponent>(m_CubeEntity, m_CubeAsset);
    } else {
        NV_LOG_ERROR("AssimpLayer: failed to load Cube primitive");
    }

    // External mesh (random AssetID).
    m_SuzanneAsset = AssetManager::Get()
        .Acquire<MeshAsset>(kSuzannePath, MeshAssetDesc{ .m_AABBTreeDepth = 1 })
        .GetAssetRef();
    m_SuzanneLoaded = m_SuzanneAsset && m_SuzanneAsset->Load();
    if (m_SuzanneLoaded) {
        m_SuzanneEntity = m_Scene.CreateEntity("Suzanne");
        registry.emplace<TransformComponent>(
            m_SuzanneEntity,
            glm::vec3(1.4f, 0.0f, 0.0f),
            glm::vec3(0.0f),
            glm::vec3(1.0f));

        Nova::Core::Renderer::RHI::MaterialGPU mat{};
        mat.m_BaseColor = glm::vec3(0.85f, 0.55f, 0.25f);
        registry.emplace<MeshRendererComponent>(m_SuzanneEntity, m_SuzanneAsset, mat);
        registry.emplace<MeshComponent>(m_SuzanneEntity, m_SuzanneAsset);
    } else {
        NV_LOG_ERROR(("AssimpLayer: failed to load '" + std::string(kSuzannePath) + "'").c_str());
    }
}

void AssimpLayer::OnDetach() {
    NV_LOG_INFO("AssimpLayer detached");

    m_SuzanneAsset.reset();
    m_CubeAsset.reset();
    m_Camera.reset();
    m_Scene.Clear();
    m_SuzanneEntity = entt::null;
    m_CubeEntity = entt::null;

    if (m_Renderer) {
        Nova::Core::Application::Get().GetImGuiLayer().DestroyImGuiBackend(Nova::Core::GraphicsAPI::Vulkan);
        m_Renderer->Destroy();
        m_Renderer.reset();
    }
}

void AssimpLayer::OnUpdate(float dt) {
    m_DeltaTime = dt;
    m_ElapsedTime += dt;

    if (m_Renderer)
        m_Renderer->Update(dt);

    auto& registry = m_Scene.GetRegistry();
    if (m_SuzanneEntity != entt::null) {
        if (auto* transform = registry.try_get<Nova::Core::ECS::Components::TransformComponent>(m_SuzanneEntity))
            transform->m_Rotation.y = m_ElapsedTime * 0.6f;
    }
    if (m_CubeEntity != entt::null) {
        if (auto* transform = registry.try_get<Nova::Core::ECS::Components::TransformComponent>(m_CubeEntity))
            transform->m_Rotation.y = -m_ElapsedTime * 0.45f;
    }
}

void AssimpLayer::PushFrameUniforms() {
    if (!m_Renderer)
        return;

    auto* graph = m_Renderer->GetRenderGraph();
    if (!graph)
        return;

    auto* shader = graph->GetShader(m_SimpleLitShader);
    if (!shader)
        return;

    shader->SetParameter("m_Time", m_ElapsedTime);
    shader->SetParameter("m_TimeDelta", m_DeltaTime);
    shader->SetParameter("m_FrameRate", m_DeltaTime > 0.0f ? 1.0f / m_DeltaTime : 0.0f);
    shader->SetParameter("m_Frame", static_cast<int>(m_FrameIndex++));
    shader->SetParameter("m_Resolution", glm::vec3(m_ViewportSize.x, m_ViewportSize.y, 1.0f));
}

void AssimpLayer::PushSceneUniforms() {
    if (!m_Renderer || !m_Camera)
        return;

    auto* graph = m_Renderer->GetRenderGraph();
    const auto* engine = graph ? graph->GetEngineParameterBlock() : nullptr;
    if (!engine || !engine->m_Scene.m_Uniforms.IsValid())
        return;

    Nova::Core::Renderer::RHI::SceneUniforms scene{};
    scene.m_Camera.m_View = m_Camera->GetViewMatrix();
    scene.m_Camera.m_Proj = m_Camera->GetProjectionMatrix();
    scene.m_Camera.m_ViewProj = scene.m_Camera.m_Proj * scene.m_Camera.m_View;
    scene.m_Camera.m_InvViewProj = glm::inverse(scene.m_Camera.m_ViewProj);
    scene.m_Camera.m_Position = m_Camera->m_LookFrom;
    scene.m_LightCount = 0;

    m_Renderer->UpdateGpuBuffer(engine->m_Scene.m_Uniforms, &scene, sizeof(scene), 0);
}

void AssimpLayer::DrawMeshEntity(Nova::Core::Renderer::RHI::IPassContext& ctx, entt::entity entity) {
    if (entity == entt::null)
        return;

    auto& registry = m_Scene.GetRegistry();
    auto* mrc = registry.try_get<Nova::Core::ECS::Components::MeshRendererComponent>(entity);
    if (!mrc || !mrc->m_MeshAsset || !mrc->m_MeshAsset->IsLoaded())
        return;

    auto* shader = ctx.GetShader(m_SimpleLitShader);
    if (!shader)
        return;

    auto cpuMesh = mrc->m_MeshAsset->GetCPUMesh();
    if (!cpuMesh)
        return;

    shader->SetParameter("m_Model", m_Scene.GetWorldTransform(entity));
    shader->SetParameter("m_Base", mrc->m_Material.m_Base);
    shader->SetParameter("m_BaseColor", mrc->m_Material.m_BaseColor);

    ctx.BindShader(m_SimpleLitShader);

    Nova::Core::Renderer::RHI::RHI_DrawIndexedCommand cmd{};
    cmd.m_Mesh = cpuMesh;
    cmd.m_Topology = Nova::Core::Renderer::RHI::RHI_PrimitiveTopology::Triangles;
    cmd.m_IndexType = Nova::Core::Renderer::RHI::RHI_IndexType::UInt32;
    cmd.m_IndexCount = static_cast<uint32_t>(cpuMesh->GetIndices().size());
    ctx.DrawIndexed(cmd);
}

void AssimpLayer::RenderScene(Nova::Core::Renderer::RHI::IPassContext& ctx) {
    DrawMeshEntity(ctx, m_CubeEntity);
    DrawMeshEntity(ctx, m_SuzanneEntity);
}

void AssimpLayer::OnBegin() {
    if (!m_Renderer)
        return;

    m_Renderer->BeginFrame();
    PushFrameUniforms();
    PushSceneUniforms();
}

void AssimpLayer::OnRender() {
    if (m_Renderer)
        m_Renderer->RenderFrame();
}

void AssimpLayer::OnEnd() {
    if (m_Renderer)
        m_Renderer->EndFrame();
}

void AssimpLayer::DrawMeshInfoPanel(
    const char* name,
    const std::shared_ptr<Nova::Core::Asset::Assets::MeshAsset>& asset,
    bool loaded) const
{
    ImGui::SeparatorText(name);

    ImGui::TextColored(
        loaded ? ImVec4(0.3f, 0.9f, 0.4f, 1.0f) : ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
        loaded ? "Load: OK" : "Load: FAILED");

    if (!asset)
        return;

    ImGui::Text("AssetID: %u", asset->GetUUID());

    if (auto mesh = asset->GetCPUMesh()) {
        const size_t vertexCount = mesh->GetVertices().size();
        const size_t indexCount = mesh->GetIndices().size();
        const size_t polygonCount = indexCount / 3;
        ImGui::Text("Vertices:  %zu", vertexCount);
        ImGui::Text("Polygons:  %zu", polygonCount);
    } else {
        ImGui::TextDisabled("Vertices / polygons: n/a");
    }
}

void AssimpLayer::OnImGuiRender() {
    ImGui::Begin("Assimp Object Loading");

    ImGui::TextUnformatted("Shader: SimpleLit (hardcoded directional light)");
    ImGui::Text("Engine reserved AssetIDs: [0, %u)", Nova::Core::kEngineReservedUUIDEnd);
    ImGui::Spacing();

    DrawMeshInfoPanel("Cube (engine primitive)", m_CubeAsset, m_CubeLoaded);
    ImGui::Spacing();
    DrawMeshInfoPanel("Suzanne (external .obj)", m_SuzanneAsset, m_SuzanneLoaded);

    ImGui::End();
}

void AssimpLayer::OnEvent(Nova::Core::Events::Event& e) {
    Nova::Core::Events::EventDispatcher dispatcher(e);
    dispatcher.Dispatch<Nova::Core::Events::KeyPressedEvent>([this](Nova::Core::Events::KeyPressedEvent& ev) {
        return OnKeyPressed(ev);
    });
    dispatcher.Dispatch<Nova::Core::Events::WindowResizeEvent>([this](Nova::Core::Events::WindowResizeEvent& ev) {
        return OnWindowResized(ev);
    });
}

bool AssimpLayer::OnKeyPressed(Nova::Core::Events::KeyPressedEvent& e) {
    if (e.IsRepeat())
        return false;
    return false;
}

bool AssimpLayer::OnWindowResized(Nova::Core::Events::WindowResizeEvent& e) {
    const uint32_t w = e.GetWidth();
    const uint32_t h = e.GetHeight();
    if (w == 0 || h == 0)
        return false;

    m_ViewportSize = { static_cast<float>(w), static_cast<float>(h) };
    if (m_Camera)
        m_Camera->m_AspectRatio = m_ViewportSize.x / m_ViewportSize.y;

    if (m_Renderer) {
        m_Renderer->Resize(static_cast<int>(w), static_cast<int>(h));
        if (auto* graph = m_Renderer->GetRenderGraph())
            graph->BindEngineShadowMaps(m_ShadowMaps);
    }
    return false;
}