#ifndef APPLAYER_H
#define APPLAYER_H

#include <memory>

#include <glm/glm.hpp>

#include "imgui.h"

#include "Core/Layer.h"
#include "Events/Event.h"
#include "Events/InputEvents.h"
#include "Events/ApplicationEvents.h"

#include "Asset/Assets/TextureAsset.h"
#include "App/AppScene.h"
#include "App/CameraController.h"
#include "App/AppRenderer.h"
#include "App/RenderDebugMode.h"

namespace Nova::App {

    namespace Editor { class EditorLayer; }
    namespace Game { class GameLayer; }

    class AppLayer : public Nova::Core::Layer {
    public:
        explicit AppLayer(): Layer("AppLayer") {}
        ~AppLayer() override;

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(float dt) override;
        void OnBegin() override;
        void OnRender() override;
        void OnEnd() override;
        void OnImGuiRender() override;
        void OnEvent(Nova::Core::Events::Event& e) override;

        enum class SceneState {
            Edit = 0, Play = 1
        };
        SceneState GetSceneState() const { return m_SceneState; }
        void SetSceneState(SceneState state) { m_SceneState = state; }

        Nova::Core::Renderer::RHI::IRenderer* GetRenderer() const { return m_AppRenderer.GetRenderer(); }

        void SetViewportHovered(bool hovered) { m_ViewportHovered = hovered; }
        bool IsViewportHovered() const { return m_ViewportHovered; }

        /** Viewport cursor UV in [0,1], (0,0) = top-left of the rendered image. */
        void SetViewportCursorUV(float u, float v) { m_ViewportCursorUV = { u, v }; }

        void ShowGrid(bool show) { m_AppRenderer.SetShowGrid(show); }
        bool IsGridVisible() const { return m_AppRenderer.IsGridVisible(); }

        void ShowAABB(bool show) { m_AppRenderer.SetShowAABB(show); }
        bool IsAABBVisible() const { return m_AppRenderer.IsAABBVisible(); }

        float GetDeltaTime() const { return m_DeltaTime; }

        RenderDebugMode GetRenderDebugMode() const { return m_AppRenderer.GetRenderDebugMode(); }
        void SetRenderDebugMode(RenderDebugMode mode) { m_AppRenderer.SetRenderDebugMode(mode); }

        Nova::Core::Renderer::RHI::RHI_TextureHandle GetSceneColor() const {
            return m_AppRenderer.GetSceneColor();
        }
        Nova::Core::Renderer::RHI::RHI_TextureHandle GetSceneDepth() const {
            return m_AppRenderer.GetSceneDepth();
        }

        void* GetPlayIconImGuiID() const;
        void* GetPauseIconImGuiID() const;

        void RequestPlay();
        void RequestStop();

        void RequestViewportResize(float width, float height);
        void ApplyPendingViewportResize();

        void RegisterEditorLayer(Editor::EditorLayer* layer) { m_EditorLayer = layer; }
        void RegisterGameLayer(Game::GameLayer* layer) { m_GameLayer = layer; }

        Editor::EditorLayer* GetEditorLayer() const { return m_EditorLayer; }
        Game::GameLayer* GetGameLayer() const { return m_GameLayer; }

        const Nova::Core::Scene::Scene& GetScene() const { return m_AppScene.GetScene(); }
        Nova::Core::Scene::Scene& GetScene() { return m_AppScene.GetScene(); }

        Nova::Core::Math::Camera* GetCamera() { return m_AppScene.GetCamera(); }
        const Nova::Core::Math::Camera* GetCamera() const { return m_AppScene.GetCamera(); }

        void BeginOrbitFocus(const glm::vec3& center, const glm::vec3& extents);
        void ExitOrbitMode();

    private:
        bool OnMouseButtonPressed(Nova::Core::Events::MouseButtonPressedEvent& e);
        bool OnMouseButtonReleased(Nova::Core::Events::MouseButtonReleasedEvent& e);
        bool OnMouseMoved(Nova::Core::Events::MouseMovedEvent& e);
        bool OnMouseScrolled(Nova::Core::Events::MouseScrolledEvent& e);
        bool OnWindowResized(Nova::Core::Events::WindowResizeEvent& e);
        bool OnImGuiPanelResize(Nova::Core::Events::ImGuiPanelResizeEvent& e);

        void SetupDockSpace(ImGuiID dockspace_id);

        AppRenderer m_AppRenderer;
        AppScene m_AppScene;
        CameraController m_CameraController;

        SceneState m_SceneState{ SceneState::Edit };
        float m_DeltaTime = 0.0f;
        float m_ElapsedTime{0.0f};
        uint32_t m_FrameIndex{0};

        glm::vec2 m_ViewportSize{ 0.0f, 0.0f };
        glm::vec2 m_PendingViewportSize{ 0.0f, 0.0f };
        bool m_ViewportResizePending{ false };
        bool m_ViewportHovered{ false };
        glm::vec2 m_ViewportCursorUV{ 0.5f, 0.5f };

        Editor::EditorLayer* m_EditorLayer{ nullptr };
        Game::GameLayer* m_GameLayer{ nullptr };

        std::shared_ptr<Nova::Core::Asset::Assets::TextureAsset> m_PlayIcon;
        std::shared_ptr<Nova::Core::Asset::Assets::TextureAsset> m_PauseIcon;
    };

    extern AppLayer* g_AppLayer;

} // namespace Nova::App

#endif // APPLAYER_H