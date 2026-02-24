#include "App/GameLayer.h"
#include "App/EditorLayer.h"
#include "App/AppLayer.h"

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

    bool GameLayer::OnKeyReleased(KeyReleasedEvent& e) {
        if (e.GetKeyCode() == SDLK_SPACE) {
            Nova::Core::Application::Get().GetLayerStack().QueueLayerTransition<EditorLayer>(this);
            std::cout << "GameLayer: Transition to EditorLayer requested." << std::endl;

            Nova::App::g_AppLayer->SetSceneState(Nova::App::AppLayer::SceneState::Edit);
            return true;
        }
        return false;
    }

    void GameLayer::OnAttach() {
        if (Nova::App::g_AppLayer)
            Nova::App::g_AppLayer->RegisterGameLayer(this);
    }

    void GameLayer::OnDetach() {
        if (Nova::App::g_AppLayer)
            Nova::App::g_AppLayer->RegisterGameLayer(nullptr);
    }

    void GameLayer::OnUpdate(float dt) {
        (void)dt;
        //later
    }

    void GameLayer::OnRender() {
        auto& registry = Nova::App::g_Scene.GetRegistry();

        Nova::Core::Renderer::Graphics::Camera* cameraPtr = nullptr;
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
        Nova::App::g_AppLayer->EndScene();
    }

    void GameLayer::OnImGuiRender() {}

} // namespace Nova::App