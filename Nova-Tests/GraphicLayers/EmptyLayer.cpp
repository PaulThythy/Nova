#include "EmptyLayer.h"

#include <SDL3/SDL.h>

#include "Core/Application.h"
#include "Core/Log.h"
#include "SDLLayer.h"

#include "imgui.h"

EmptyLayer::EmptyLayer() : Layer("EmptyLayer") {}

void EmptyLayer::OnAttach() { NV_LOG_INFO("EmptyLayer attached"); }
void EmptyLayer::OnDetach() { NV_LOG_INFO("EmptyLayer detached"); }
void EmptyLayer::OnUpdate(float /*dt*/) {}
void EmptyLayer::OnBegin() {}
void EmptyLayer::OnRender() {}
void EmptyLayer::OnEnd() {}

void EmptyLayer::OnImGuiRender() {
    ImGui::Begin("EmptyLayer");
    ImGui::Text("Press SPACE to transition to SDLLayer");
    ImGui::End();
}

void EmptyLayer::OnEvent(Nova::Core::Events::Event& e) {
    Nova::Core::Events::EventDispatcher dispatcher(e);
    dispatcher.Dispatch<Nova::Core::Events::KeyPressedEvent>([this](Nova::Core::Events::KeyPressedEvent& ev) {
        return OnKeyPressed(ev);
    });
}

bool EmptyLayer::OnKeyPressed(Nova::Core::Events::KeyPressedEvent& e) {
    if (e.IsRepeat())
        return false;

    if (e.GetKeyCode() == SDLK_SPACE) {
        Nova::Core::Application::Get().GetLayerStack().QueueLayerTransition<SDLLayer>(this);
        NV_LOG_INFO("EmptyLayer: transition to SDLLayer requested");
        return true;
    }

    return false;
}