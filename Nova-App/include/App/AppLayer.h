#ifndef APPLAYER_H
#define APPLAYER_H

#include <memory>
#include <entt/entt.hpp>
#include <glad/gl.h>
#include <SDL3/SDL.h>

#include "Core/Application.h"
#include "Core/Layer.h"
#include "Scene/Scene.h"

#include "Scene/ECS/Components/TransformComponent.h"
#include "Scene/ECS/Components/MeshComponent.h"

#include "Renderer/Mesh.h"
#include "Renderer/OpenGL/GL_Mesh.h"
#include "Renderer/OpenGL/GL_Shader.h"

#include "Events/Event.h"
#include "Events/InputEvents.h"

using namespace Nova::Core::Events;
using namespace Nova::Core;

namespace Nova::App {

    class AppLayer : public Layer {
    public:
        explicit AppLayer(): Layer("AppLayer") {}
        ~AppLayer() override;

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(float dt) override;
        void OnRender() override;
        void OnImGuiRender() override;
        void OnEvent(Event& e) override;
    };

    extern Nova::Core::Scene::Scene g_Scene;

} // namespace Nova::App

#endif // APPLAYER_H