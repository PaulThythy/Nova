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
        const float axisLength = 1.0f;

        // X axis : red
        m_XAxis = std::make_unique<Line>(
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(axisLength, 0.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f)
        );

        // Y axis : green
        m_YAxis = std::make_unique<Line>(
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, axisLength),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        // Z axis : blue
        m_ZAxis = std::make_unique<Line>(
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, axisLength, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );

        m_Grid = std::make_unique<Grid>(
            10,
            1.0f,
            glm::vec3(0.3f)
        );
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
            m_Grid->m_MVP = viewProj;
            m_Grid->draw();
        }
        if (m_XAxis) {
            m_XAxis->m_MVP = viewProj;
            m_XAxis->draw();
        }
        if (m_YAxis) {
            m_YAxis->m_MVP = viewProj;
            m_YAxis->draw();
        }
        if (m_ZAxis) {
            m_ZAxis->m_MVP = viewProj;
            m_ZAxis->draw();
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