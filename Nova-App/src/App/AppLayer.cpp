#include "App/AppLayer.h"

#include <cmath>
#include <filesystem>
#include <iostream>

#include "imgui_internal.h"

#include "Editor/EditorLayer.h"
#include "Game/GameLayer.h"

#include "Asset/AssetManager.h"
#include "Asset/Assets/TextureAsset.h"
#include "Core/Application.h"
#include "Core/Assert.h"
#include "Core/Log.h"

#include "UI/Panels/AssetBrowserPanel.h"
#include "UI/Panels/HierarchyPanel.h"
#include "UI/Panels/InspectorPanel.h"
#include "UI/Panels/MainMenuBar.h"
#include "UI/Panels/ScenePanel.h"

namespace Nova::App {

    using namespace Nova::Core::Asset;
    using namespace Nova::Core::Asset::Assets;
    using namespace Nova::Core::Events;

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

        ImGui::DockBuilderDockWindow(m_AppScene.GetName().c_str(), dock_left);
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

        Nova::Core::Application::Get().GetLayerStack().QueueLayerTransition<Game::GameLayer>(m_EditorLayer);
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

        Nova::Core::Application::Get().GetLayerStack().QueueLayerTransition<Editor::EditorLayer>(m_GameLayer);
        NV_LOG_DEBUG("[AppLayer] Transition to EditorLayer requested.");

        SetSceneState(SceneState::Edit);
    }

    void* AppLayer::GetPlayIconImGuiID() const {
        if (!m_PlayIcon)
            return nullptr;
        auto* renderer = m_AppRenderer.GetRenderer();
        if (!renderer)
            return nullptr;
        return renderer->GetTextureImGuiID(m_PlayIcon->GetCPUTexture());
    }

    void* AppLayer::GetPauseIconImGuiID() const {
        if (!m_PauseIcon)
            return nullptr;
        auto* renderer = m_AppRenderer.GetRenderer();
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

        m_AppRenderer.Initialize(
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
                auto* renderer = m_AppRenderer.GetRenderer();
                if (!renderer || !renderer->GetOrUploadTexture(asset->GetCPUTexture())) {
                    NV_LOG_WARN(("Failed to upload texture asset: " + uri.generic_string()).c_str());
                    return nullptr;
                }
                return asset;
            };
            m_PlayIcon = acquireTexture("Editor://Icons/play-6-48.png");
            m_PauseIcon = acquireTexture("Editor://Icons/pause-48.png");
        }

        m_AppScene.SetupDefaultScene();
        m_CameraController.SetNavigationFromCamera(*m_AppScene.GetCamera());
    }

    void AppLayer::OnDetach() {
        m_PlayIcon.reset();
        m_PauseIcon.reset();
        m_AppRenderer.Shutdown();
        m_AppScene.Clear();

        if (g_AppLayer == this)
            g_AppLayer = nullptr;
    }

    void AppLayer::OnUpdate(float dt) {
        m_DeltaTime = dt;
        m_ElapsedTime += dt;

        if (m_AppScene.GetCamera())
            m_CameraController.Update(dt, *m_AppScene.GetCamera());
    }

    void AppLayer::OnBegin() {
        NV_ASSERT_MSG(m_AppScene.GetCamera(), "Camera is not initialized.");

        if (m_ViewportResizePending)
            ApplyPendingViewportResize();

        m_AppRenderer.BeginFrame();
        Editor::EditorSelection* selection = m_EditorLayer ? &m_EditorLayer->GetSelection() : nullptr;
        m_AppRenderer.BindFrame(m_AppScene.GetScene(), *m_AppScene.GetCamera(), selection);
        m_AppRenderer.UploadLights();
        m_AppRenderer.PushGlobals(m_ElapsedTime, m_DeltaTime, m_FrameIndex, m_ViewportSize);
    }

    void AppLayer::OnRender() {
        NV_ASSERT_MSG(m_AppScene.GetCamera(), "Camera is not initialized.");
        m_AppRenderer.RenderFrame();
    }

    void AppLayer::OnEnd() {
        NV_ASSERT_MSG(m_AppScene.GetCamera(), "Camera is not initialized.");
        m_AppRenderer.EndFrame();
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

        UI::Panels::ScenePanel::Render(m_AppScene.GetName());
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
        if (!m_AppScene.GetCamera())
            return false;
        return m_CameraController.OnMouseMoved(e, *m_AppScene.GetCamera());
    }

    bool AppLayer::OnMouseScrolled(MouseScrolledEvent& e) {
        if (!m_AppScene.GetCamera())
            return false;
        return m_CameraController.OnMouseScrolled(e, m_ViewportHovered, *m_AppScene.GetCamera(), m_ViewportCursorUV.x, m_ViewportCursorUV.y);
    }

    bool AppLayer::OnWindowResized(WindowResizeEvent& e) {
        RequestViewportResize(static_cast<float>(e.GetWidth()), static_cast<float>(e.GetHeight()));
        return false;
    }

    bool AppLayer::OnImGuiPanelResize(ImGuiPanelResizeEvent& e) {
        RequestViewportResize(e.GetWidth(), e.GetHeight());
        return false;
    }

    void AppLayer::BeginOrbitFocus(const glm::vec3& center, const glm::vec3& extents) {
        if (!m_AppScene.GetCamera())
            return;
        m_CameraController.BeginOrbitFocus(center, extents, *m_AppScene.GetCamera());
    }

    void AppLayer::ExitOrbitMode() {
        if (!m_AppScene.GetCamera())
            return;
        m_CameraController.ExitOrbitMode(*m_AppScene.GetCamera());
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
        m_AppRenderer.Resize(newW, newH);
        m_AppRenderer.RebindAfterResize();

        if (m_AppScene.GetCamera())
            m_AppScene.GetCamera()->m_AspectRatio = static_cast<float>(newW) / static_cast<float>(newH);

        m_ViewportResizePending = false;
    }

} // namespace Nova::App