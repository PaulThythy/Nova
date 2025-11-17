#include "App/SpiralGalaxyLayer.h"

#include <iostream>

namespace Nova::App {

    SpiralGalaxyLayer::~SpiralGalaxyLayer() = default;

    void SpiralGalaxyLayer::OnEvent() {}

    void SpiralGalaxyLayer::OnAttach() {
        // Load shader
        std::string novaAppRootDir = NOVA_APP_ROOT_DIR;
        m_SpiralGalaxyProgram = Nova::Renderer::OpenGL::LoadRenderShader(
            novaAppRootDir + "/shaders/OpenGL/spiralGalaxy/spiralGalaxy.vert",
            novaAppRootDir + "/shaders/OpenGL/spiralGalaxy/spiralGalaxy.frag"
        );

        if (!m_SpiralGalaxyProgram) {
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

    void SpiralGalaxyLayer::OnDetach() {
        if (m_SpiralGalaxyProgram) {
            glDeleteProgram(m_SpiralGalaxyProgram);
            m_SpiralGalaxyProgram = 0;
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

    void SpiralGalaxyLayer::OnUpdate(float dt) {
        m_Time += dt;

        float mx = 0.0f, my = 0.0f;
        Uint32 buttons = SDL_GetMouseState(&mx, &my);

        bool leftDown = (buttons & SDL_BUTTON_LMASK) != 0;

        auto& window = Core::Application::Get().GetWindow();
        int w, h;
        window.GetWindowSize(w, h);

        if (leftDown) {
            // If we just clicked, record the click position
            if (!m_MouseDown) {
                m_MouseClickPos = ImVec2((float)mx, (float)my);
            }
            m_MouseDown = true;
            m_MousePos = ImVec2((float)mx, (float)my);
        }
        else {
            m_MouseDown = false;
            // We keep m_MouseClickPos, but the current position for iMouse.xy will be (0,0)
            m_MousePos = ImVec2((float)mx, (float)my);
        }
    }

    void SpiralGalaxyLayer::OnRender() {
        if (!m_SpiralGalaxyProgram)
            return;

        auto& window = Core::Application::Get().GetWindow();
        int w, h;
        window.GetWindowSize(w, h);

        glUseProgram(m_SpiralGalaxyProgram);

        // Uniforms Shadertoy
        GLint locResolution = glGetUniformLocation(m_SpiralGalaxyProgram, "iResolution");
        GLint locTime = glGetUniformLocation(m_SpiralGalaxyProgram, "iTime");
        GLint locMouse = glGetUniformLocation(m_SpiralGalaxyProgram, "iMouse");

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

        glBindVertexArray(m_VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
    }

    void SpiralGalaxyLayer::OnImGuiRender() {
        ImGui::Begin("Spiral galaxy");

        ImGuiIO& io = ImGui::GetIO();
        float fps = io.Framerate;
        float msPerFrame = fps > 0.0f ? 1000.0f / fps : 0.0f;

        ImGui::Separator();
        ImGui::Text("Frame time: %.3f ms (%.1f FPS)", msPerFrame, fps);

        ImGui::End();
    }
}