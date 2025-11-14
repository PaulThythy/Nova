#ifndef APP_LAYER_H
#define APP_LAYER_H

#include <glad/gl.h>
#include <SDL3/SDL.h>
#include "imgui.h"

#include "Core/Application.h"
#include "Core/Layer.h"
#include "Renderer/OpenGL/GL_Shader.h"

namespace Nova::App {

    class AppLayer : public Nova::Core::Layer {
    public:
        explicit AppLayer(Nova::Core::Application& app) : Nova::Core::Layer("AppLayer"), m_App(app) {}
        ~AppLayer() override;

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(float dt) override;
        void OnRender() override;
        void OnImGuiRender() override;
        void OnEvent(/*Nova::Core::Event& e*/) override;

    private:
        Nova::Core::Application& m_App;

        GLuint m_VAO = 0;
        GLuint m_VBO = 0;
        GLuint m_SeascapeProgram = 0;

        // uniforms / state
        float m_Time      = 0.0f;
        float m_DeltaTime = 0.0f;
        int   m_Frame     = 0;

        bool  m_MouseDown = false;
        ImVec2 m_MousePos{0.0f, 0.0f};
        ImVec2 m_MouseClickPos{0.0f, 0.0f};

        float m_SeaHeight = 0.6f;
        float m_SeaChoppy = 4.0f;
        float m_SeaSpeed  = 0.8f;
        float m_SeaFreq   = 0.16f;
    };

} // namespace Nova::App

#endif // APP_LAYER_H