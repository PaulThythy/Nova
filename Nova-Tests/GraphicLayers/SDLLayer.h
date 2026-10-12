#ifndef SDLLAYER_H
#define SDLLAYER_H

#include <SDL3/SDL.h>

#include "Core/Layer.h"
#include "Events/Event.h"
#include "Events/InputEvents.h"

class SDLLayer : public Nova::Core::Layer {
public:
    explicit SDLLayer();
    ~SDLLayer() override = default;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float dt) override;
    void OnBegin() override;
    void OnRender() override;
    void OnEnd() override;
    void OnImGuiRender() override;
    void OnEvent(Nova::Core::Events::Event& e) override;

private:
    bool OnKeyPressed(Nova::Core::Events::KeyPressedEvent& e);

    SDL_Renderer* m_Renderer = nullptr;
};

#endif // SDLLAYER_H