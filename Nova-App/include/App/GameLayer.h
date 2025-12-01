#ifndef GAMELAYER_H
#define GAMELAYER_H

#include <iostream>
#include <glad/gl.h>

#include "Events/Event.h"
#include "Events/InputEvents.h"

#include "Core/Layer.h"
#include "Core/Application.h"

#include "Scene/ECS/Components/CameraComponent.h"
#include "Scene/ECS/Components/TransformComponent.h"
#include "Scene/ECS/Components/MeshComponent.h"

using namespace Nova::Core::Events;
using namespace Nova::Core;

namespace Nova::App {

    class GameLayer : public Layer {
    public:
        explicit GameLayer(): Layer("GameLayer") {}
        ~GameLayer() override;

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

#endif // GAMELAYER_H