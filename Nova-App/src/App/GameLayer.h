#ifndef GAMELAYER_H
#define GAMELAYER_H

#include "Core/Layer.h"
#include "Events/Event.h"

namespace Nova::App {

    class AppLayer;

    class GameLayer : public Nova::Core::Layer {
    public:
        explicit GameLayer(): Layer("GameLayer") {}
        ~GameLayer() override = default;

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

#endif // GAMELAYER_H