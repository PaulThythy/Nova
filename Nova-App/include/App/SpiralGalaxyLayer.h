#ifndef SPIRALGALAXY_LAYER_H
#define SPIRALGALAXY_LAYER_H

#include <glad/gl.h>
#include <SDL3/SDL.h>
#include "imgui.h"

#include "Core/Application.h"
#include "Core/Layer.h"
#include "Renderer/OpenGL/GL_Shader.h"

#include "Events/Event.h"
#include "Events/InputEvents.h"

using namespace Nova::Core::Events;

namespace Nova::App {

    class SpiralGalaxyLayer : public Nova::Core::Layer {
    public:
        explicit SpiralGalaxyLayer() : Nova::Core::Layer("SpiralGalaxyLayer") {}
        ~SpiralGalaxyLayer() override;

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(float dt) override;
        void OnRender() override;
        void OnImGuiRender() override;
        void OnEvent(Event& e) override;

    private:
        GLuint m_VAO = 0;
        GLuint m_VBO = 0;
        GLuint m_SpiralGalaxyProgram = 0;

        // uniforms / state
        float m_Time = 0.0f;
        float m_DeltaTime = 0.0f;
        int   m_Frame = 0;

        bool  m_MouseDown = false;
        ImVec2 m_MousePos{ 0.0f, 0.0f };
        ImVec2 m_MouseClickPos{ 0.0f, 0.0f };

        bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
        bool OnMouseButtonReleased(MouseButtonReleasedEvent& e);
        bool OnMouseMoved(MouseMovedEvent& e);
        bool OnKeyReleased(KeyReleasedEvent& e);
    };

} // namespace Nova::App

#endif // SPIRALGALAXY_LAYER_H