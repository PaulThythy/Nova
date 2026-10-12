#include "SDLLayer.h"

#include <SDL3/SDL.h>

#include "Core/Application.h"
#include "Core/GraphicsAPI.h"
#include "Core/Log.h"
#include "VulkanLayer.h"

#include "imgui.h"

SDLLayer::SDLLayer() : Layer("SDLLayer") {}

void SDLLayer::OnAttach() {
    NV_LOG_INFO("SDLLayer attached");

    auto& app = Nova::Core::Application::Get();
    auto& window = app.GetWindow();

    const auto graphicsAPI = Nova::Core::GraphicsAPI::SDLRenderer;

    if (window.GetGraphicsAPI() != graphicsAPI) {
        if (!window.SetGraphicsAPI(graphicsAPI)) {
            NV_LOG_ERROR("SDLLayer: failed to set GraphicsAPI::SDLRenderer");
            return;
        }
    }

    app.GetImGuiLayer().SetImGuiBackend(graphicsAPI);

    m_Renderer = window.GetSDLRenderer();
    if (!m_Renderer) {
        NV_LOG_ERROR("SDLLayer: SDL_Renderer* is null after SetGraphicsAPI");
        return;
    }

    NV_LOG_INFO("SDLLayer: SDL_Renderer ready");
}

void SDLLayer::OnDetach() {
    NV_LOG_INFO("SDLLayer detached");

    // Release ImGui's SDLRenderer backend while the SDL_Renderer is still alive.
    Nova::Core::Application::Get().GetImGuiLayer().DestroyImGuiBackend(
        Nova::Core::GraphicsAPI::SDLRenderer);

    m_Renderer = nullptr;
}

void SDLLayer::OnUpdate(float /*dt*/) {}
void SDLLayer::OnBegin() {}

void SDLLayer::OnRender() {
    if (!m_Renderer)
        return;

    SDL_SetRenderDrawColor(m_Renderer, 220, 70, 70, 255);
    const SDL_FRect rect{ 120.0f, 120.0f, 240.0f, 240.0f };
    SDL_RenderFillRect(m_Renderer, &rect);
}

void SDLLayer::OnEnd() {}

void SDLLayer::OnImGuiRender() {
    ImGui::Begin("SDLLayer");
    ImGui::Text("Graphics API: SDLRenderer");
    ImGui::Text("SDL_Renderer: %p", static_cast<void*>(m_Renderer));
    ImGui::Text("Press SPACE to transition to VulkanLayer");
    ImGui::End();
}

void SDLLayer::OnEvent(Nova::Core::Events::Event& e) {
    Nova::Core::Events::EventDispatcher dispatcher(e);
    dispatcher.Dispatch<Nova::Core::Events::KeyPressedEvent>([this](Nova::Core::Events::KeyPressedEvent& ev) {
        return OnKeyPressed(ev);
    });
}

bool SDLLayer::OnKeyPressed(Nova::Core::Events::KeyPressedEvent& e) {
    if (e.IsRepeat())
        return false;

    if (e.GetKeyCode() == SDLK_SPACE) {
        Nova::Core::Application::Get().GetLayerStack().QueueLayerTransition<VulkanLayer>(this);
        NV_LOG_INFO("SDLLayer: transition to VulkanLayer requested");
        return true;
    }

    return false;
}