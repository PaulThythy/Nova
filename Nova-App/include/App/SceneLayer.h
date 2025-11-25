#ifndef SCENELAYER_H
#define SCENELAYER_H

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

    class SceneLayer : public Layer {
    public:
        explicit SceneLayer(): Layer("SceneLayer") {}
        ~SceneLayer() override;

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(float dt) override;
        void OnRender() override;
        void OnImGuiRender() override;
        void OnEvent(Event& e) override;

    private:
        GLuint m_SceneProgram{ 0 };

        Nova::Core::Scene::Scene m_Scene;

        bool OnKeyReleased(KeyReleasedEvent& e);
    };

} // namespace Nova::App

#endif // SCENELAYER_H