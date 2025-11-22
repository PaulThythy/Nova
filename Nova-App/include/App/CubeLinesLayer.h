#ifndef CUBELINES_LAYER_H
#define CUBELINES_LAYER_H

#include <glad/gl.h>
#include <SDL3/SDL.h>
#include "imgui.h"

#include "Core/Application.h"
#include "Core/Layer.h"
#include "Renderer/OpenGL/GL_Shader.h"

#include "Events/Event.h"
#include "Events/InputEvents.h"

using namespace Nova::Events;

namespace Nova::App {

    class CubeLinesLayer : public Nova::Core::Layer {
    public:
        explicit CubeLinesLayer() : Nova::Core::Layer("CubeLinesLayer") {}
        ~CubeLinesLayer() override;

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(float dt) override;
        void OnRender() override;
        void OnImGuiRender() override;
        void OnEvent(Event& e) override;

    private:
        GLuint m_VAO = 0;
        GLuint m_VBO = 0;
        GLuint m_CubeLinesProgram = 0;

        // uniforms / state
        float m_Time = 0.0f;
        float m_DeltaTime = 0.0f;
        int   m_Frame = 0;

        bool  m_MouseDown = false;
        ImVec2 m_MousePos{ 0.0f, 0.0f };
        ImVec2 m_MouseClickPos{ 0.0f, 0.0f };

        bool OnMouseButtonPressed(Nova::Events::MouseButtonPressedEvent& e);
        bool OnMouseButtonReleased(Nova::Events::MouseButtonReleasedEvent& e);
        bool OnMouseMoved(Nova::Events::MouseMovedEvent& e);
        bool OnKeyReleased(Nova::Events::KeyReleasedEvent& e);
    };

} // namespace Nova::App

#endif // CUBELINES_LAYER_H