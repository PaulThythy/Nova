#ifndef APPLAYER_H
#define APPLAYER_H

#include <memory>
#include <optional>
#include <vector>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include "imgui.h"

#include "Core/Layer.h"
#include "Events/Event.h"
#include "Events/InputEvents.h"
#include "Events/ApplicationEvents.h"

#include "Asset/Assets/TextureAsset.h"
#include "Math/Camera.h"
#include "Renderer/RHI/RHI_Renderer.h"
#include "Scene/Scene.h"

#include "App/EditorSelection.h"
#include "App/CameraController.h"
#include "App/EditorRenderer.h"
#include "App/RenderDebugMode.h"

namespace Nova::App {

    class EditorLayer;
    class GameLayer;

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

        Nova::Core::Renderer::RHI::IRenderer* GetRenderer() const { return m_EditorRenderer.GetRenderer(); }

        void SetViewportHovered(bool hovered) { m_ViewportHovered = hovered; }
        bool IsViewportHovered() const { return m_ViewportHovered; }

        /** Viewport cursor UV in [0,1], (0,0) = top-left of the rendered image. */
        void SetViewportCursorUV(float u, float v) { m_ViewportCursorUV = { u, v }; }

        void ShowGrid(bool show) { m_EditorRenderer.SetShowGrid(show); }
        bool IsGridVisible() const { return m_EditorRenderer.IsGridVisible(); }

        void ShowAABB(bool show) { m_EditorRenderer.SetShowAABB(show); }
        bool IsAABBVisible() const { return m_EditorRenderer.IsAABBVisible(); }

        float GetDeltaTime() const { return m_DeltaTime; }

        RenderDebugMode GetRenderDebugMode() const { return m_EditorRenderer.GetRenderDebugMode(); }
        void SetRenderDebugMode(RenderDebugMode mode) { m_EditorRenderer.SetRenderDebugMode(mode); }

        Nova::Core::Renderer::RHI::RHI_TextureHandle GetSceneColor() const {
            return m_EditorRenderer.GetSceneColor();
        }
        Nova::Core::Renderer::RHI::RHI_TextureHandle GetSceneDepth() const {
            return m_EditorRenderer.GetSceneDepth();
        }

        void* GetPlayIconImGuiID() const;
        void* GetPauseIconImGuiID() const;

        void RequestPlay();
        void RequestStop();

        void RequestViewportResize(float width, float height);
        void ApplyPendingViewportResize();

        void RegisterEditorLayer(EditorLayer* layer) { m_EditorLayer = layer; }
        void RegisterGameLayer(GameLayer* layer) { m_GameLayer = layer; }

        const Nova::Core::Scene::Scene& GetScene() const { return m_Scene; }
        Nova::Core::Scene::Scene& GetScene() { return m_Scene; }

        void PickAtViewportUV(float u, float v, bool addToSelection = false);
        void FocusAtViewportUV(float u, float v);
        void ClearFocus();

        entt::entity GetFocusedEntity() const { return m_Selection.GetFocused(); }
        bool HasFocus() const { return m_Selection.HasFocus(); }
        std::optional<FocusInfo> GetFocusInfo() const { return m_Selection.GetFocusInfo(); }

        const std::vector<entt::entity>& GetSelectedEntities() const {
            return m_Selection.GetEntities();
        }
        entt::entity GetSelectedEntity() const { return m_Selection.GetSelected(); }
        void SetSelectedEntity(entt::entity entity) { m_Selection.SetSelected(entity); }
        void ClearSelection() { m_Selection.Clear(); }
        bool IsSelected(entt::entity entity) const { return m_Selection.IsSelected(entity); }

    private:
        bool OnMouseButtonPressed(Nova::Core::Events::MouseButtonPressedEvent& e);
        bool OnMouseButtonReleased(Nova::Core::Events::MouseButtonReleasedEvent& e);
        bool OnMouseMoved(Nova::Core::Events::MouseMovedEvent& e);
        bool OnMouseScrolled(Nova::Core::Events::MouseScrolledEvent& e);
        bool OnKeyPressed(Nova::Core::Events::KeyPressedEvent& e);
        bool OnWindowResized(Nova::Core::Events::WindowResizeEvent& e);
        bool OnImGuiPanelResize(Nova::Core::Events::ImGuiPanelResizeEvent& e);

        void SetupDockSpace(ImGuiID dockspace_id);
        void SetupDefaultScene();

        EditorRenderer m_EditorRenderer;
        CameraController m_CameraController;
        EditorSelection m_Selection;

        SceneState m_SceneState{ SceneState::Edit };
        Nova::Core::Scene::Scene m_Scene{"Scene_test"};
        float m_DeltaTime = 0.0f;
        float m_ElapsedTime{0.0f};
        uint32_t m_FrameIndex{0};

        std::shared_ptr<Nova::Core::Math::Camera> m_Camera;

        glm::vec2 m_ViewportSize{ 0.0f, 0.0f };
        glm::vec2 m_PendingViewportSize{ 0.0f, 0.0f };
        bool m_ViewportResizePending{ false };
        bool m_ViewportHovered{ false };
        glm::vec2 m_ViewportCursorUV{ 0.5f, 0.5f };

        EditorLayer* m_EditorLayer{ nullptr };
        GameLayer* m_GameLayer{ nullptr };

        std::shared_ptr<Nova::Core::Asset::Assets::TextureAsset> m_PlayIcon;
        std::shared_ptr<Nova::Core::Asset::Assets::TextureAsset> m_PauseIcon;
    };

    extern AppLayer* g_AppLayer;

} // namespace Nova::App

#endif // APPLAYER_H
