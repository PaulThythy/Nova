#ifndef EMPTYLAYER_H
#define EMPTYLAYER_H

#include "Core/Layer.h"
#include "Events/Event.h"
#include "Events/InputEvents.h"

class EmptyLayer : public Nova::Core::Layer {
public:
    explicit EmptyLayer();
    ~EmptyLayer() override = default;

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
};

#endif // EMPTYLAYER_H