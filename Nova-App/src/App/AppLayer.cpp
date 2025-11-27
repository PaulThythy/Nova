#include "App/AppLayer.h"

#include "imgui.h"

#include "Scene/ECS/Components/TransformComponent.h"
#include "Scene/ECS/Components/MeshComponent.h"

#include <iostream>

namespace Nova::App {

    AppLayer::~AppLayer() = default;

    void AppLayer::OnEvent(Event& e) {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<KeyReleasedEvent>(
            [this](KeyReleasedEvent& ev) { return OnKeyReleased(ev); });
    }

    void AppLayer::OnAttach() {
        std::string root = NOVA_APP_ROOT_DIR;
        m_SceneProgram = Nova::Core::Renderer::OpenGL::LoadRenderShader(
            root + "/shaders/OpenGL/scene/scene.vert",
            root + "/shaders/OpenGL/scene/scene.frag"
        );

        if (!m_SceneProgram) {
            std::cerr << "Failed to load scene shader program\n";
        }

        std::shared_ptr<Nova::Core::Renderer::Mesh> cpuPlane = Nova::Core::Renderer::Mesh::CreatePlane();
        std::shared_ptr<Nova::Core::Renderer::Mesh> glPlane = std::make_shared<Nova::Core::Renderer::OpenGL::GL_Mesh>(*cpuPlane);

        entt::entity planeEntity = m_Scene.CreateEntity("Plane");

        auto& registry = m_Scene.GetRegistry();

        registry.emplace<Scene::ECS::Components::TransformComponent>(planeEntity);
        registry.emplace<Scene::ECS::Components::MeshComponent>(planeEntity, glPlane);

        glPlane->Upload(*cpuPlane);
    }

    void AppLayer::OnDetach() {
        auto& registry = m_Scene.GetRegistry();
        registry.clear();

        if (m_SceneProgram) {
            glDeleteProgram(m_SceneProgram);
            m_SceneProgram = 0;
        }
    }

    void AppLayer::OnUpdate(float dt) {
        (void)dt;
        //later
    }

    void AppLayer::OnRender() {
        if (!m_SceneProgram)
            return;

        glUseProgram(m_SceneProgram);

        auto& registry = m_Scene.GetRegistry();

        auto view = registry.view<Nova::Core::Scene::ECS::Components::TransformComponent, Nova::Core::Scene::ECS::Components::MeshComponent>();
        for (auto entity : view) {
            auto& transform = view.get<Nova::Core::Scene::ECS::Components::TransformComponent>(entity);
            auto& meshComp = view.get<Nova::Core::Scene::ECS::Components::MeshComponent>(entity);

            if (!meshComp.m_Mesh)
                continue;

            meshComp.m_Mesh->Bind();
            meshComp.m_Mesh->Draw();
            meshComp.m_Mesh->Unbind();
        }
    }

    void AppLayer::OnImGuiRender() {
        ImGui::Begin("SceneLayer");
        ImGui::Text("Scene shader: %s", m_SceneProgram ? "loaded" : "NOT loaded");
        ImGui::End();
    }

    bool AppLayer::OnKeyReleased(KeyReleasedEvent& e) {
        /*if (e.GetKeyCode() == SDLK_SPACE) {
            Nova::Core::Application::Get().GetLayerStack().QueueLayerTransition<SingularityLayer>(this);
            std::cout << "SceneLayer: Transition to SingularityLayer requested." << std::endl;
            return true;
        }*/
        return false;
    }

} // namespace Nova::App