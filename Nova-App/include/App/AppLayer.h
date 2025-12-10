#ifndef APPLAYER_H
#define APPLAYER_H

#include <memory>
#include <entt/entt.hpp>
#include <glad/gl.h>
#include <SDL3/SDL.h>

#include <glm/glm.hpp>

#include "imgui.h"
#include "imgui_internal.h"

#include "Core/Application.h"
#include "Core/Layer.h"
#include "Scene/Scene.h"

#include "Scene/ECS/Components/TransformComponent.h"
#include "Scene/ECS/Components/MeshComponent.h"

#include "Renderer/OpenGL/GL_Mesh.h"
#include "Renderer/OpenGL/GL_Shader.h"
#include "Renderer/OpenGL/GL_FrameBuffer.h"

#include "Events/Event.h"
#include "Events/InputEvents.h"
#include "Events/ApplicationEvents.h"

#include "UI/Panels/HierarchyPanel.h"
#include "UI/Panels/ViewportPanel.h"
#include "UI/Panels/InspectorPanel.h"
#include "UI/Panels/AssetBrowserPanel.h"
#include "UI/Panels/MainMenuBar.h"

using namespace Nova::Core::Events;
using namespace Nova::Core;

namespace Nova::App {

    class AppLayer : public Layer {
    public:
        explicit AppLayer(): Layer("AppLayer") {}
        ~AppLayer() override;

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(float dt) override;
        void OnRender() override;
        void OnImGuiRender() override;
        void OnEvent(Event& e) override;

        void BeginScene();
        void RenderScene(const glm::mat4& viewProj);
        void EndScene();

        void SetViewportSize(float width, float height);
        GLuint GetViewportTexture() const { 
            return m_FrameBuffer ? m_FrameBuffer->GetColorAttachment() : 0;
        }

    private: 
        void SetupDockSpace(ImGuiID dockspace_id);

        GLuint m_SceneProgram{ 0 };

        glm::vec2 m_ViewportSize{ 0.0f, 0.0f };
        glm::vec2 m_PendingViewportSize{ 0.0f, 0.0f };
        bool      m_ViewportResizePending{ false };

        std::unique_ptr<Nova::Core::Renderer::OpenGL::GL_FrameBuffer> m_FrameBuffer;
        
        bool OnImGuiPanelResize(ImGuiPanelResizeEvent& e);
    };

    extern Scene::Scene g_Scene;
    extern AppLayer* g_AppLayer;

} // namespace Nova::App

#endif // APPLAYER_H