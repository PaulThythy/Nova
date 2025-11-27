#include "App/EditorLayer.h"
#include "App/GameLayer.h"

#include "imgui.h"

namespace Nova::App {

    EditorLayer::~EditorLayer() = default;

    void EditorLayer::OnEvent(Event& e) {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<KeyReleasedEvent>(
            [this](KeyReleasedEvent& ev) { return OnKeyReleased(ev); });
    }

    void EditorLayer::OnAttach() {

    }

    void EditorLayer::OnDetach() {

    }

    void EditorLayer::OnUpdate(float dt) {
        (void)dt;
        //later
    }

    void EditorLayer::OnRender() {

    }

    void EditorLayer::OnImGuiRender() {
        ImGui::Begin("Editor Layer");
        ImGui::Text("Editor Layer");
        ImGui::End();
    }

    bool EditorLayer::OnKeyReleased(KeyReleasedEvent& e) {
        if (e.GetKeyCode() == SDLK_SPACE) {
            Nova::Core::Application::Get().GetLayerStack().QueueLayerTransition<GameLayer>(this);
            std::cout << "EditorLayer: Transition to GameLayer requested." << std::endl;
            return true;
        }
        return false;
    }

} // namespace Nova::App