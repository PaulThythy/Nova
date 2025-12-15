#include "App/EditorLayer.h"
#include "App/AppLayer.h"
#include "App/GameLayer.h"

#include "Scene/ECS/Components/CameraComponent.h"

#include "imgui.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Nova::App {

    using Nova::Core::Scene::ECS::Components::CameraComponent;
    using Nova::Core::Scene::ECS::Components::TransformComponent;
    using Nova::Core::Scene::ECS::Components::MeshComponent;

    EditorLayer::~EditorLayer() = default;

    void EditorLayer::OnEvent(Event& e) {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<KeyReleasedEvent>(
            [this](KeyReleasedEvent& ev) { return OnKeyReleased(ev); });
    }

    void EditorLayer::OnAttach() {
        m_Grid = std::make_unique<Grid>();
    }

    void EditorLayer::OnDetach() {}

    void EditorLayer::OnUpdate(float dt) {
        (void)dt;
        //later
    }

    void EditorLayer::OnRender() {
        auto& registry = Nova::App::g_Scene.GetRegistry();

        Nova::Core::Renderer::Camera* cameraPtr = nullptr;
        auto camView = registry.view<CameraComponent>();
        for (auto entity : camView) {
            auto& camComp = camView.get<CameraComponent>(entity);
            if (camComp.m_Camera && camComp.m_IsPrimary) {
                cameraPtr = camComp.m_Camera.get();
                break;
            }
        }

        if (!cameraPtr)
            return;

        glm::mat4 view = cameraPtr->GetViewMatrix();
        glm::mat4 projection = cameraPtr->GetProjectionMatrix();
        glm::mat4 viewProj = projection * view;

        Nova::App::g_AppLayer->BeginScene();
        Nova::App::g_AppLayer->RenderScene(viewProj);

        glDisable(GL_DEPTH_TEST);

        //model matrix is identity
        if (m_Grid) {
            glm::mat4 invView = glm::inverse(view);
            glm::vec3 camPos = glm::vec3(invView[3]);

            m_Grid->m_ViewProj = viewProj;
            m_Grid->m_InvViewProj = glm::inverse(viewProj);
            m_Grid->m_CameraPos = camPos;

            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            m_Grid->draw();

            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
        }

        glEnable(GL_DEPTH_TEST);

        Nova::App::g_AppLayer->EndScene();
    }

    void EditorLayer::OnImGuiRender() {}

    bool EditorLayer::OnKeyReleased(KeyReleasedEvent& e) {
        if (e.GetKeyCode() == SDLK_SPACE) {
            Nova::Core::Application::Get().GetLayerStack().QueueLayerTransition<GameLayer>(this);
            std::cout << "EditorLayer: Transition to GameLayer requested." << std::endl;
            return true;
        }
        return false;
    }

} // namespace Nova::App