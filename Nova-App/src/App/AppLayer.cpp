#include "App/AppLayer.h"

#include <iostream>

namespace Nova::App {

    AppLayer::~AppLayer() = default;

    void AppLayer::OnEvent() {}

    void AppLayer::OnAttach() {
        // Load shader
        std::string novaAppRootDir = NOVA_APP_ROOT_DIR;
        m_SeascapeProgram = Nova::Renderer::OpenGL::LoadRenderShader(
            novaAppRootDir + "/shaders/OpenGL/seascape/seascape.vert",
            novaAppRootDir + "/shaders/OpenGL/seascape/seascape.frag"
        );

        if (!m_SeascapeProgram) {
            std::cerr << "Failed to load seascape shader program\n";
        }

        float vertices[] = {
            // positions   // uv
            -1.0f, -1.0f,  0.0f, 0.0f,
             3.0f, -1.0f,  2.0f, 0.0f,
            -1.0f,  3.0f,  0.0f, 2.0f
        };

        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);

        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void AppLayer::OnDetach() {
        if (m_SeascapeProgram) {
            glDeleteProgram(m_SeascapeProgram);
            m_SeascapeProgram = 0;
        }
        if (m_VBO) {
            glDeleteBuffers(1, &m_VBO);
            m_VBO = 0;
        }
        if (m_VAO) {
            glDeleteVertexArrays(1, &m_VAO);
            m_VAO = 0;
        }
    }

    void AppLayer::OnUpdate(float dt) {
        m_Time += dt;

        float mx = 0.0f, my = 0.0f;
        Uint32 buttons = SDL_GetMouseState(&mx, &my);

        bool leftDown = (buttons & SDL_BUTTON_LMASK) != 0;

        auto& window = m_App.GetWindow();
        int w, h;
        window.GetWindowSize(w, h);

        if (leftDown) {
            // If we just clicked, record the click position
            if (!m_MouseDown) {
                m_MouseClickPos = ImVec2((float)mx, (float)my);
            }
            m_MouseDown = true;
            m_MousePos = ImVec2((float)mx, (float)my);
        } else {
            m_MouseDown = false;
            // We keep m_MouseClickPos, but the current position for iMouse.xy will be (0,0)
            m_MousePos = ImVec2((float)mx, (float)my);
        }
    }

    void AppLayer::OnRender() {
        if (!m_SeascapeProgram)
            return;

        auto& window = m_App.GetWindow();
        int w, h;
        window.GetWindowSize(w, h);

        glUseProgram(m_SeascapeProgram);

        // Uniforms Shadertoy
        GLint locResolution = glGetUniformLocation(m_SeascapeProgram, "iResolution");
        GLint locTime       = glGetUniformLocation(m_SeascapeProgram, "iTime");
        GLint locMouse      = glGetUniformLocation(m_SeascapeProgram, "iMouse");

        if (locResolution >= 0)
            glUniform3f(locResolution, (float)w, (float)h, 1.0f);

        if (locTime >= 0)
            glUniform1f(locTime, m_Time);

        if (locMouse >= 0) {
            ImGuiIO& io = ImGui::GetIO();

            float curX = 0.0f;
            float curY = 0.0f;

            // Si ImGui ne veut PAS la souris et que le bouton gauche est enfoncé,
            // alors on laisse la caméra du shader utiliser la souris.
            if (!io.WantCaptureMouse && m_MouseDown) {
                curX = m_MousePos.x;
                curY = (float)h - m_MousePos.y; // flip Y pour Shadertoy
            }

            float clickX = m_MouseClickPos.x;
            float clickY = (float)h - m_MouseClickPos.y;

            glUniform4f(locMouse, curX, curY, clickX, clickY);
        }

        // Uniforms de contrôle de la mer
        GLint locSeaHeight = glGetUniformLocation(m_SeascapeProgram, "uSeaHeight");
        GLint locSeaChoppy = glGetUniformLocation(m_SeascapeProgram, "uSeaChoppy");
        GLint locSeaSpeed  = glGetUniformLocation(m_SeascapeProgram, "uSeaSpeed");
        GLint locSeaFreq   = glGetUniformLocation(m_SeascapeProgram, "uSeaFreq");

        if (locSeaHeight >= 0) glUniform1f(locSeaHeight, m_SeaHeight);
        if (locSeaChoppy >= 0) glUniform1f(locSeaChoppy, m_SeaChoppy);
        if (locSeaSpeed  >= 0) glUniform1f(locSeaSpeed,  m_SeaSpeed);
        if (locSeaFreq   >= 0) glUniform1f(locSeaFreq,   m_SeaFreq);

        glBindVertexArray(m_VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
    }

    void AppLayer::OnImGuiRender() {
        ImGui::Begin("Seascape Controls");

        ImGui::SliderFloat("Sea Speed",   &m_SeaSpeed,  0.0f, 3.0f);
        ImGui::SliderFloat("Sea Height",  &m_SeaHeight, 0.0f, 2.0f);
        ImGui::SliderFloat("Sea Choppy",  &m_SeaChoppy, 0.0f, 8.0f);
        ImGui::SliderFloat("Sea frequency", &m_SeaFreq, 0.0f, 1.0f);

        ImGuiIO& io = ImGui::GetIO();
        float fps = io.Framerate;
        float msPerFrame = fps > 0.0f ? 1000.0f / fps : 0.0f;

        ImGui::Separator();
        ImGui::Text("Frame time: %.3f ms (%.1f FPS)", msPerFrame, fps);

        ImGui::End();
    }
}