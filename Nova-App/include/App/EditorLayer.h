#ifndef EDITORLAYER_H
#define EDITORLAYER_H

#include <iostream>

#include "Events/Event.h"
#include "Events/InputEvents.h"

#include "Core/Layer.h"
#include "Core/Application.h"

using namespace Nova::Core::Events;
using namespace Nova::Core;

namespace Nova::App {

    class EditorLayer : public Layer {
    public:
        explicit EditorLayer(): Layer("EditorLayer") {}
        ~EditorLayer() override;

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(float dt) override;
        void OnRender() override;
        void OnImGuiRender() override;
        void OnEvent(Event& e) override;

    private:
        bool OnKeyReleased(KeyReleasedEvent& e);
    };

} // namespace Nova::App

#endif // EDITORLAYER_H