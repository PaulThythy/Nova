#include "App/AppLayer.h"

#include <cmath>
#include <filesystem>
#include <iostream>

#include <SDL3/SDL.h>
#include "imgui_internal.h"

#include "App/EditorLayer.h"
#include "App/GameLayer.h"

#include "Asset/AssetManager.h"
#include "Asset/Assets/MeshAsset.h"
#include "Asset/Assets/TextureAsset.h"
#include "Core/Application.h"
#include "Core/Assert.h"
#include "Core/Log.h"
#include "ECS/Components/CameraComponent.h"
#include "ECS/Components/LightComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/MeshRendererComponent.h"
#include "ECS/Components/NameComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "Math/Light.h"

#include "UI/Panels/AssetBrowserPanel.h"
#include "UI/Panels/HierarchyPanel.h"
#include "UI/Panels/InspectorPanel.h"
#include "UI/Panels/MainMenuBar.h"
#include "UI/Panels/ScenePanel.h"

namespace Nova::App {

    using namespace Nova::Core::Asset;
    using namespace Nova::Core::Asset::Assets;
    using namespace Nova::Core::ECS::Components;
    using namespace Nova::Core::Events;
    using namespace Nova::Core::Math;

    AppLayer* g_AppLayer = nullptr;

    AppLayer::~AppLayer() = default;

    void AppLayer::SetupDockSpace(ImGuiID dockspace_id) {
        static bool s_DockInitialized = false;
        if (s_DockInitialized)
            return;

        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_None);
        ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

        ImGuiID dock_down = 0;
        ImGuiID dock_center = 0;
        ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.19f, &dock_down, &dock_center);

        ImGuiID dock_right = 0;
        ImGuiID dock_left = 0;
        ImGui::DockBuilderSplitNode(dock_center, ImGuiDir_Right, 0.27f, &dock_right, &dock_left);

        ImGuiID dock_right_top = 0;
        ImGuiID dock_right_bottom = 0;
        ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Up, 0.50f, &dock_right_top, &dock_right_bottom);

        ImGui::DockBuilderDockWindow(m_Scene.GetName().c_str(), dock_left);
        ImGui::DockBuilderDockWindow("Hierarchy", dock_right_top);
        ImGui::DockBuilderDockWindow("Inspector", dock_right_bottom);
        ImGui::DockBuilderDockWindow("Asset Browser", dock_down);

        ImGui::DockBuilderFinish(dockspace_id);
        s_DockInitialized = true;
    }

    void AppLayer::OnEvent(Event& e) {
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
        dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev) {
            return OnKeyPressed(ev);
        });
        dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) {
            return OnWindowResized(ev);
        });
        dispatcher.Dispatch<ImGuiPanelResizeEvent>([this](ImGuiPanelResizeEvent& ev) {
            return OnImGuiPanelResize(ev);
        });
    }

    void AppLayer::RequestPlay() {
        if (m_SceneState == SceneState::Play)
            return;

        if (!m_EditorLayer) {
            NV_LOG_ERROR("[AppLayer] Cannot Play: EditorLayer not registered.");
            return;
        }

        Nova::Core::Application::Get().GetLayerStack().QueueLayerTransition<GameLayer>(m_EditorLayer);
        NV_LOG_DEBUG("[AppLayer] Transition to GameLayer requested.");

        SetSceneState(SceneState::Play);
    }

    void AppLayer::RequestStop() {
        if (m_SceneState == SceneState::Edit)
            return;

        if (!m_GameLayer) {
            NV_LOG_ERROR("[AppLayer] Cannot Stop: GameLayer not registered.");
            return;
        }

        Nova::Core::Application::Get().GetLayerStack().QueueLayerTransition<EditorLayer>(m_GameLayer);
        NV_LOG_DEBUG("[AppLayer] Transition to EditorLayer requested.");

        SetSceneState(SceneState::Edit);
    }

    void* AppLayer::GetPlayIconImGuiID() const {
        if (!m_PlayIcon)
            return nullptr;
        auto* renderer = m_EditorRenderer.GetRenderer();
        if (!renderer)
            return nullptr;
        return renderer->GetTextureImGuiID(m_PlayIcon->GetCPUTexture());
    }

    void* AppLayer::GetPauseIconImGuiID() const {
        if (!m_PauseIcon)
            return nullptr;
        auto* renderer = m_EditorRenderer.GetRenderer();
        if (!renderer)
            return nullptr;
        return renderer->GetTextureImGuiID(m_PauseIcon->GetCPUTexture());
    }

    void AppLayer::OnAttach() {
        g_AppLayer = this;

        Nova::Core::GraphicsAPI api = Nova::Core::Application::Get().GetWindow().GetGraphicsAPI();

        int initialW = 1280, initialH = 720;
        Nova::Core::Application::Get().GetWindow().GetWindowSize(initialW, initialH);
        if (initialW <= 0 || initialH <= 0) {
            initialW = 1280;
            initialH = 720;
        }
        m_ViewportSize = { static_cast<float>(initialW), static_cast<float>(initialH) };

        m_EditorRenderer.Initialize(
            api,
            static_cast<uint32_t>(initialW),
            static_cast<uint32_t>(initialH));

        {
            auto acquireTexture = [this](const std::filesystem::path& uri) -> std::shared_ptr<TextureAsset> {
                auto asset = AssetManager::Get().Acquire<TextureAsset>(uri).GetAssetRef();
                if (!asset || !asset->Load()) {
                    NV_LOG_WARN(("Failed to load texture asset: " + uri.generic_string()).c_str());
                    return nullptr;
                }
                auto* renderer = m_EditorRenderer.GetRenderer();
                if (!renderer || !renderer->GetOrUploadTexture(asset->GetCPUTexture())) {
                    NV_LOG_WARN(("Failed to upload texture asset: " + uri.generic_string()).c_str());
                    return nullptr;
                }
                return asset;
            };
            m_PlayIcon = acquireTexture("Editor://Icons/play-6-48.png");
            m_PauseIcon = acquireTexture("Editor://Icons/pause-48.png");
        }

        SetupDefaultScene();
        m_CameraController.SetNavigationFromCamera(*m_Camera);
    }

    void AppLayer::SetupDefaultScene() {
        m_Camera = std::make_shared<Camera>(
            glm::vec3(5.0f, 5.0f, 5.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            45.0f,
            16.0f / 9.0f,
            0.1f,
            100.0f,
            true
        );
        m_Camera->m_IsPerspective = true;
        m_Camera->m_FOV = 45.0f;
        m_Camera->m_NearPlane = 0.1f;
        m_Camera->m_FarPlane = 1000.0f;
        m_Camera->m_Up = {0.0f, 1.0f, 0.0f};

        entt::entity cameraEntity = m_Scene.CreateEntity("Camera");
        m_Scene.SetMainCamera(cameraEntity);

        auto& registry = m_Scene.GetRegistry();
        registry.emplace<CameraComponent>(cameraEntity, m_Camera, true);

        auto cubeAsset = AssetManager::Get().Acquire<MeshAsset>(
            "Engine://Primitives/Cube", MeshAssetDesc{ .m_AABBTreeDepth = 1 }).GetAssetRef();
        cubeAsset->Load();
        entt::entity cubeEntity = m_Scene.CreateEntity("Cube");
        registry.emplace<TransformComponent>(cubeEntity,
            glm::vec3(0.0f, 0.5f, 0.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 1.0f));
        {
            Nova::Core::Renderer::RHI::Material mat{};
            mat.m_BaseColor = glm::vec3(0.0f, 1.0f, 0.0f);
            registry.emplace<MeshRendererComponent>(cubeEntity, cubeAsset, mat);
            registry.emplace<MeshComponent>(cubeEntity, cubeAsset);
        }

        auto torusAsset = AssetManager::Get().Acquire<MeshAsset>(
            "Engine://Primitives/Torus", MeshAssetDesc{ .m_AABBTreeDepth = 1 }).GetAssetRef();
        torusAsset->Load();
        entt::entity torusEntity = m_Scene.CreateEntity("Torus");
        registry.emplace<TransformComponent>(torusEntity,
            glm::vec3(2.0f, 0.25f, 1.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 1.0f));
        {
            Nova::Core::Renderer::RHI::Material mat{};
            mat.m_BaseColor = glm::vec3(1.0f, 0.5f, 0.0f);
            registry.emplace<MeshRendererComponent>(torusEntity, torusAsset, mat);
            registry.emplace<MeshComponent>(torusEntity, torusAsset);
        }

        auto sphereAsset = AssetManager::Get().Acquire<MeshAsset>(
            "Engine://Primitives/Sphere", MeshAssetDesc{ .m_AABBTreeDepth = 1 }).GetAssetRef();
        sphereAsset->Load();
        entt::entity sphereEntity = m_Scene.CreateEntity("Sphere");
        registry.emplace<TransformComponent>(sphereEntity,
            glm::vec3(0.0f, 0.5f, -1.5f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 1.0f));
        {
            Nova::Core::Renderer::RHI::Material mat{};
            mat.m_BaseColor = glm::vec3(0.0f, 0.0f, 1.0f);
            registry.emplace<MeshRendererComponent>(sphereEntity, sphereAsset, mat);
            registry.emplace<MeshComponent>(sphereEntity, sphereAsset);
        }

        auto planeAsset = AssetManager::Get().Acquire<MeshAsset>(
            "Engine://Primitives/Plane", MeshAssetDesc{ .m_AABBTreeDepth = 1 }).GetAssetRef();
        planeAsset->Load();
        entt::entity planeEntity = m_Scene.CreateEntity("Plane");
        registry.emplace<TransformComponent>(planeEntity,
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(10.0f, 10.0f, 10.0f));
        {
            Nova::Core::Renderer::RHI::Material mat{};
            registry.emplace<MeshRendererComponent>(planeEntity, planeAsset, mat);
            registry.emplace<MeshComponent>(planeEntity, planeAsset);
        }

        {
            entt::entity dirLightEntity = m_Scene.CreateEntity("DirectionalLight");
            registry.emplace<TransformComponent>(dirLightEntity,
                glm::vec3(0.0f, 8.0f, 0.0f),
                glm::vec3(glm::radians(-45.0f), glm::radians(45.0f), 0.0f),
                glm::vec3(1.0f));
            auto dirLight = std::make_shared<Light>();
            dirLight->m_Type = LightType::Directional;
            dirLight->m_Color = glm::vec3(1.0f);
            dirLight->m_Intensity = 3.0f;
            dirLight->m_Direction = glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f));
            dirLight->m_LightShadow = true;
            dirLight->m_ShadowBiasConstant = 0.5f;
            dirLight->m_ShadowBiasSlope = 1.0f;
            dirLight->m_ShadowNormalBias = 0.012f;
            registry.emplace<LightComponent>(dirLightEntity, dirLight);
        }

        {
            entt::entity spotEntity = m_Scene.CreateEntity("SpotLight");
            registry.emplace<TransformComponent>(spotEntity,
                glm::vec3(2.0f, 6.0f, 2.0f),
                glm::vec3(glm::radians(-60.0f), glm::radians(-20.0f), 0.0f),
                glm::vec3(1.0f));
            auto spot = std::make_shared<Light>();
            spot->m_Type = LightType::Spot;
            spot->m_Color = glm::vec3(1.0f, 1.0f, 1.0f);
            spot->m_Intensity = 8.0f;
            spot->m_Direction = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.2f));
            spot->m_Range = 20.0f;
            spot->m_InnerCone = 15.0f;
            spot->m_OuterCone = 30.0f;
            spot->m_LightShadow = true;
            spot->m_ShadowBiasConstant = 0.2f;
            spot->m_ShadowBiasSlope = 0.4f;
            spot->m_ShadowNormalBias = 0.003f;
            registry.emplace<LightComponent>(spotEntity, spot);
        }
    }

    void AppLayer::OnDetach() {
        m_PlayIcon.reset();
        m_PauseIcon.reset();
        m_EditorRenderer.Shutdown();
        m_Scene.Clear();

        if (g_AppLayer == this)
            g_AppLayer = nullptr;
    }

    void AppLayer::OnUpdate(float dt) {
        m_DeltaTime = dt;
        m_ElapsedTime += dt;

        if (m_Camera)
            m_CameraController.Update(dt, *m_Camera);
    }

    void AppLayer::OnBegin() {
        NV_ASSERT_MSG(m_Camera, "Camera is not initialized.");

        if (m_ViewportResizePending)
            ApplyPendingViewportResize();

        m_EditorRenderer.BeginFrame();
        m_EditorRenderer.BindFrame(m_Scene, *m_Camera, m_Selection);
        m_EditorRenderer.UploadLights();
        m_EditorRenderer.PushGlobals(m_ElapsedTime, m_DeltaTime, m_FrameIndex, m_ViewportSize);
    }

    void AppLayer::OnRender() {
        NV_ASSERT_MSG(m_Camera, "Camera is not initialized.");
        m_EditorRenderer.RenderFrame();
    }

    void AppLayer::OnEnd() {
        NV_ASSERT_MSG(m_Camera, "Camera is not initialized.");
        m_EditorRenderer.EndFrame();
    }

    void AppLayer::OnImGuiRender() {
        UI::Panels::MainMenuBar::Render();

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGuiWindowFlags host_flags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_NoDocking;

        ImGui::Begin("Nova Editor", nullptr, host_flags);
        ImGui::PopStyleVar(3);

        ImGuiID dockspace_id = ImGui::GetID("NovaDockspace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

        SetupDockSpace(dockspace_id);
        ImGui::End();

        UI::Panels::ScenePanel::Render(m_Scene.GetName());
        UI::Panels::HierarchyPanel::Render();
        UI::Panels::InspectorPanel::Render();
        UI::Panels::AssetBrowserPanel::Render();
    }

    bool AppLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e) {
        return m_CameraController.OnMouseButtonPressed(e, m_ViewportHovered);
    }

    bool AppLayer::OnMouseButtonReleased(MouseButtonReleasedEvent& e) {
        return m_CameraController.OnMouseButtonReleased(e);
    }

    bool AppLayer::OnMouseMoved(MouseMovedEvent& e) {
        if (!m_Camera)
            return false;
        return m_CameraController.OnMouseMoved(e, *m_Camera);
    }

    bool AppLayer::OnMouseScrolled(MouseScrolledEvent& e) {
        if (!m_Camera)
            return false;
        return m_CameraController.OnMouseScrolled(e, m_ViewportHovered, *m_Camera, m_ViewportCursorUV.x, m_ViewportCursorUV.y);
    }

    bool AppLayer::OnKeyPressed(KeyPressedEvent& e) {
        if (e.IsRepeat())
            return false;

        if (e.GetKeyCode() == SDLK_ESCAPE) {
            ClearFocus();
            return true;
        }
        return false;
    }

    bool AppLayer::OnWindowResized(WindowResizeEvent& e) {
        RequestViewportResize(static_cast<float>(e.GetWidth()), static_cast<float>(e.GetHeight()));
        return false;
    }

    bool AppLayer::OnImGuiPanelResize(ImGuiPanelResizeEvent& e) {
        RequestViewportResize(e.GetWidth(), e.GetHeight());
        return false;
    }

    void AppLayer::PickAtViewportUV(float u, float v, bool addToSelection) {
        if (!m_Camera)
            return;
        m_Selection.PickAtViewportUV(m_Scene, *m_Camera, u, v, addToSelection);
    }

    void AppLayer::FocusAtViewportUV(float u, float v) {
        if (!m_Camera)
            return;

        Nova::Core::Math::AABB worldAabb{};
        const entt::entity entity = EditorSelection::PickEntityAtViewportUV(
            m_Scene, *m_Camera, u, v, &worldAabb);
        if (entity == entt::null)
            return;

        if (!EditorSelection::ComputeEntityWorldAABB(m_Scene, entity, worldAabb))
            return;

        std::string name = "unnamed";
        if (auto* nc = m_Scene.GetRegistry().try_get<Nova::Core::ECS::Components::NameComponent>(entity))
            name = nc->m_Name;

        uint32_t triangleCount = 0;
        if (auto* mc = m_Scene.GetRegistry().try_get<Nova::Core::ECS::Components::MeshComponent>(entity)) {
            if (mc->m_MeshAsset && mc->m_MeshAsset->IsLoaded()) {
                if (auto cpuMesh = mc->m_MeshAsset->GetCPUMesh())
                    triangleCount = static_cast<uint32_t>(cpuMesh->GetIndices().size() / 3);
            }
        }

        const glm::vec3 center = worldAabb.GetCenter();
        const glm::vec3 extents = worldAabb.GetExtents();

        m_Selection.SetFocused(entity, center, extents, name, triangleCount);
        m_CameraController.BeginOrbitFocus(center, extents, *m_Camera);

        std::cout << "[Focus] \"" << name << "\" center=("
                  << center.x << ", " << center.y << ", " << center.z << ")\n";
    }

    void AppLayer::ClearFocus() {
        m_Selection.Clear();
        if (m_Camera)
            m_CameraController.ExitOrbitMode(*m_Camera);
    }

    void AppLayer::RequestViewportResize(float width, float height) {
        if (width <= 0.0f || height <= 0.0f)
            return;

        const int newW = static_cast<int>(std::lround(width));
        const int newH = static_cast<int>(std::lround(height));
        if (newW <= 0 || newH <= 0)
            return;

        const int pendingW = static_cast<int>(std::lround(m_PendingViewportSize.x));
        const int pendingH = static_cast<int>(std::lround(m_PendingViewportSize.y));
        if (newW == pendingW && newH == pendingH && m_ViewportResizePending)
            return;

        const int currentW = static_cast<int>(std::lround(m_ViewportSize.x));
        const int currentH = static_cast<int>(std::lround(m_ViewportSize.y));
        if (newW == currentW && newH == currentH && !m_ViewportResizePending)
            return;

        m_PendingViewportSize = { static_cast<float>(newW), static_cast<float>(newH) };
        m_ViewportResizePending = true;
    }

    void AppLayer::ApplyPendingViewportResize() {
        const int newW = static_cast<int>(std::lround(m_PendingViewportSize.x));
        const int newH = static_cast<int>(std::lround(m_PendingViewportSize.y));
        if (newW <= 0 || newH <= 0) {
            m_ViewportResizePending = false;
            return;
        }

        const int oldW = static_cast<int>(std::lround(m_ViewportSize.x));
        const int oldH = static_cast<int>(std::lround(m_ViewportSize.y));
        if (newW == oldW && newH == oldH) {
            m_ViewportResizePending = false;
            return;
        }

        m_ViewportSize = { static_cast<float>(newW), static_cast<float>(newH) };
        m_EditorRenderer.Resize(newW, newH);
        m_EditorRenderer.RebindAfterResize();

        if (m_Camera)
            m_Camera->m_AspectRatio = static_cast<float>(newW) / static_cast<float>(newH);

        m_ViewportResizePending = false;
    }

} // namespace Nova::App