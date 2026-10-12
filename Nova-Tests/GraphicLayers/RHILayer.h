#ifndef RHILAYER_H
#define RHILAYER_H

#include <SDL3/SDL.h>

#include "Core/Layer.h"
#include "Events/Event.h"
#include "Events/InputEvents.h"
#include "Renderer/RHI/RHI_Renderer.h"

class RHILayer : public Nova::Core::Layer {
public:
    explicit RHILayer();
    ~RHILayer() override = default;

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

    std::unique_ptr<Nova::Core::Renderer::RHI::IRenderer> m_Renderer;
};

#endif // RHILAYER_H