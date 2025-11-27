#include "App/GameLayer.h"
#include "App/EditorLayer.h"

#include "imgui.h"

namespace Nova::App {

    GameLayer::~GameLayer() = default;

    void GameLayer::OnEvent(Event& e) {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<KeyReleasedEvent>(
            [this](KeyReleasedEvent& ev) { return OnKeyReleased(ev); });
    }

    void GameLayer::OnAttach() {

    }

    void GameLayer::OnDetach() {

    }

    void GameLayer::OnUpdate(float dt) {
        (void)dt;
        //later
    }

    void GameLayer::OnRender() {

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