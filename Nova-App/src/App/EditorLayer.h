#ifndef EDITORLAYER_H
#define EDITORLAYER_H

#include "Core/Layer.h"
#include "Events/Event.h"

namespace Nova::App {

    class AppLayer;

    class EditorLayer : public Nova::Core::Layer {
    public:
        explicit EditorLayer(): Layer("EditorLayer") {}
        ~EditorLayer() override = default;

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(float dt) override;
        void OnBegin() override;
        void OnRender() override;
        void OnEnd() override;
        void OnImGuiRender() override;
        void OnEvent(Nova::Core::Events::Event& e) override;
    };

} // namespace Nova::App

#endif // EDITORLAYER_H