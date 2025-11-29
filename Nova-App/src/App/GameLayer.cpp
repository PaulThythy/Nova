#include "App/GameLayer.h"
#include "App/EditorLayer.h"

#include "imgui.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Nova::App {

    using Nova::Core::Scene::ECS::Components::CameraComponent;
    using Nova::Core::Scene::ECS::Components::TransformComponent;
    using Nova::Core::Scene::ECS::Components::MeshComponent;

    GameLayer::~GameLayer() = default;

    void GameLayer::OnEvent(Event& e) {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<KeyReleasedEvent>(
            [this](KeyReleasedEvent& ev) { return OnKeyReleased(ev); });
    }

    void GameLayer::OnAttach() {
        std::string root = NOVA_APP_ROOT_DIR;
        m_SceneProgram = Nova::Core::Renderer::OpenGL::LoadRenderShader(
            root + "/shaders/OpenGL/scene/scene.vert",
            root + "/shaders/OpenGL/scene/scene.frag"
        );

        if (!m_SceneProgram) {
            std::cerr << "Failed to load scene shader program\n";
        }
    }

    void GameLayer::OnDetach() {
        if (m_SceneProgram) {
            glDeleteProgram(m_SceneProgram);
            m_SceneProgram = 0;
        }
    }

    void GameLayer::OnUpdate(float dt) {
        (void)dt;
        //later
    }

    void GameLayer::OnRender() {
        auto& registry = Nova::App::g_Scene.GetRegistry();

        entt::entity activeCam = entt::null;
        auto camView = registry.view<CameraComponent>();

        Nova::Core::Renderer::Camera* cameraPtr = nullptr;

        for (auto entity : camView) {
            auto& camComp = camView.get<CameraComponent>(entity);
            if (camComp.m_Camera && camComp.m_IsPrimary) {
                cameraPtr = camComp.m_Camera.get();
                break;
            }
        }

        if (!cameraPtr) {
            return;
        }

        glm::mat4 view       = cameraPtr->GetViewMatrix();
        glm::mat4 projection = cameraPtr->GetProjectionMatrix();
        glm::mat4 viewProj   = projection * view;

        if (m_SceneProgram) {
            glUseProgram(m_SceneProgram);

            GLint locVP = glGetUniformLocation(m_SceneProgram, "u_ViewProjection");
            if (locVP != -1) {
                glUniformMatrix4fv(locVP, 1, GL_FALSE, glm::value_ptr(viewProj));
            }

            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glFrontFace(GL_CCW);

            auto viewMeshes = registry.view<TransformComponent, MeshComponent>();
            for (auto entity : viewMeshes) {
                auto& transform = viewMeshes.get<TransformComponent>(entity);
                auto& meshComp  = viewMeshes.get<MeshComponent>(entity);

                if (!meshComp.m_Mesh)
                    continue;

                glm::mat4 model = transform.GetTransform();

                GLint locModel = glGetUniformLocation(m_SceneProgram, "u_Model");
                if (locModel != -1) {
                    glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
                }

                meshComp.m_Mesh->Bind();
                meshComp.m_Mesh->Draw();
                meshComp.m_Mesh->Unbind();
            }

            glDisable(GL_CULL_FACE);
        }
    }

    void GameLayer::OnImGuiRender() {
        ImGui::Begin("Game Layer");
        ImGui::Text("Game Layer");
        ImGui::End();
    }

    bool GameLayer::OnKeyReleased(KeyReleasedEvent& e) {
        if (e.GetKeyCode() == SDLK_SPACE) {
            Nova::Core::Application::Get().GetLayerStack().QueueLayerTransition<EditorLayer>(this);
            std::cout << "GameLayer: Transition to EditorLayer requested." << std::endl;
            return true;
        }
        return false;
    }

} // namespace Nova::App