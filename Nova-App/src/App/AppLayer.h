#ifndef APPLAYER_H
#define APPLAYER_H

#include <memory>
#include <entt/entt.hpp>
#include <SDL3/SDL.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include "imgui_internal.h"

#include "Core/Application.h"
#include "Core/Layer.h"
#include "Core/Assert.h"

#include "Asset/AssetManager.h"
#include "Asset/Assets/MeshAsset.h"
#include "Asset/Assets/ShaderAsset.h"

#include "Scene/Scene.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/MeshRendererComponent.h"
#include "ECS/Components/CameraComponent.h"

#include "Renderer/RHI/RHI_Renderer.h"
#include "Renderer/RHI/RHI_RenderGraph.h"

#include "Events/Event.h"
#include "Events/InputEvents.h"
#include "Events/ApplicationEvents.h"

#include "UI/Panels/HierarchyPanel.h"
#include "UI/Panels/InspectorPanel.h"
#include "UI/Panels/AssetBrowserPanel.h"
#include "UI/Panels/MainMenuBar.h"
#include "UI/Panels/ScenePanel.h"

using namespace Nova::Core;
using namespace Nova::Core::Events;
using namespace Nova::Core::Scene;
using namespace Nova::Core::Renderer;
using namespace Nova::Core::Renderer::Graphics;
using namespace Nova::Core::ECS::Components;

using namespace Nova::Core::Asset;
using namespace Nova::Core::Asset::Assets;

namespace Nova::App {

    enum class RenderDebugMode : int {
        Lit = 0,
        Normals,
        Positions,
        VertexColor,
        Depth,
        Count
    };

    class EditorLayer;
    class GameLayer;

    class AppLayer : public Layer {
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
        void OnEvent(Event& e) override;

        // ---- Scene rendering (Begin/End from AppLayer; RenderScene from the RG pass) ----
        void BeginRenderScene();
        void RenderScene(Nova::Core::Renderer::RHI::RHI_PassContext& ctx);
        void EndRenderScene();

        enum class SceneState {
            Edit = 0, Play = 1
        };
        SceneState GetSceneState() const { return m_SceneState; }
        void SetSceneState(SceneState state) { m_SceneState = state; }

        Nova::Core::Renderer::RHI::IRenderer* GetRenderer() const { return m_Renderer.get(); }

        // Called each frame by ScenePanel to indicate whether the mouse hovers the rendered viewport.
        void SetViewportHovered(bool hovered) { m_ViewportHovered = hovered; }
        bool IsViewportHovered() const        { return m_ViewportHovered; }

        void ShowGrid(bool show) { m_ShowGrid = show; }
        bool IsGridVisible() const { return m_ShowGrid; }

        RenderDebugMode GetRenderDebugMode() const { return m_RenderDebugMode; }
        void SetRenderDebugMode(RenderDebugMode mode) { m_RenderDebugMode = mode; }

        Nova::Core::Renderer::RHI::RHI_TextureHandle GetSceneColor() const { return m_SceneColor; }
        Nova::Core::Renderer::RHI::RHI_TextureHandle GetSceneDepth() const { return m_SceneDepth; }

        void RequestPlay();
        void RequestStop();

        void RequestViewportResize(float width, float height);
        void ApplyPendingViewportResize();

        void RegisterEditorLayer(EditorLayer* layer) { m_EditorLayer = layer; }
        void RegisterGameLayer(GameLayer* layer) { m_GameLayer = layer; }

        const Nova::Core::Scene::Scene& GetScene() const { return m_Scene; }
        Nova::Core::Scene::Scene& GetScene() { return m_Scene; }
    
    private:
        // ---- Orbit camera helpers ----
		void UpdateCameraFromOrbit();

        // ---- Mouse event handlers ----
        bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
		bool OnMouseButtonReleased(MouseButtonReleasedEvent& e);
		bool OnMouseMoved(MouseMovedEvent& e);
		bool OnMouseScrolled(MouseScrolledEvent& e);
		bool OnWindowResized(WindowResizeEvent& e);
		bool OnImGuiPanelResize(ImGuiPanelResizeEvent& e);

    private: 
        std::unique_ptr<Nova::Core::Renderer::RHI::IRenderer> m_Renderer;
        Nova::Core::Renderer::RHI::RHI_ShaderHandle GetActiveSceneShader() const;

        SceneState  m_SceneState{ SceneState::Edit };
        Nova::Core::Scene::Scene m_Scene{"Scene_test"};
        float m_DeltaTime = 0.0f;
        float m_ElapsedTime{0.0f};
		uint32_t m_FrameIndex{0};

        // ---- Camera ----
        std::shared_ptr<Camera> m_Camera;

        // ---- Orbit camera state ----
        struct OrbitState {
			glm::vec3 m_Target{0.0f, 0.0f, 0.0f};
			float m_Distance = 18.0f;

			float m_Yaw = glm::radians(35.0f);
			float m_Pitch = glm::radians(25.0f);

			float m_RotateSensitivity = 0.025f;
			float m_ZoomSensitivity = 0.5f;

			bool m_IsRotating = false;
			bool m_HasLastMousePos = false;
			glm::vec2 m_LastMousePos{0.0f, 0.0f};
		} m_Orbit;

        void SetupDockSpace(ImGuiID dockspace_id);

        Nova::Core::Renderer::RHI::RHI_TextureHandle m_SceneColor{};
        Nova::Core::Renderer::RHI::RHI_TextureHandle m_SceneDepth{};
        Nova::Core::Renderer::RHI::RHI_ShaderHandle m_GridShader{};
        Nova::Core::Renderer::RHI::RHI_ShaderHandle m_SceneShader{};
        Nova::Core::Renderer::RHI::RHI_ShaderHandle m_NormalsShader{};
        Nova::Core::Renderer::RHI::RHI_ShaderHandle m_PositionsShader{};
        Nova::Core::Renderer::RHI::RHI_ShaderHandle m_VertexColorShader{};
        Nova::Core::Renderer::RHI::RHI_ShaderHandle m_DepthShader{};

        RenderDebugMode m_RenderDebugMode = RenderDebugMode::Lit;
        bool m_ShowGrid = true;

        glm::vec2 m_ViewportSize{ 0.0f, 0.0f };
        glm::vec2 m_PendingViewportSize{ 0.0f, 0.0f };
        bool      m_ViewportResizePending{ false };
        bool      m_ViewportHovered{ false };

        EditorLayer* m_EditorLayer{ nullptr };
        GameLayer* m_GameLayer{ nullptr };
    };

    extern AppLayer* g_AppLayer;

} // namespace Nova::App

#endif // APPLAYER_H