#include "App/SeascapeLayer.h"
#include "App/SpiralGalaxyLayer.h"

#include <iostream>

namespace Nova::App {

    SeascapeLayer::~SeascapeLayer() = default;

    void SeascapeLayer::OnEvent(Event& e) {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<MouseButtonPressedEvent>(
            [this](MouseButtonPressedEvent& ev) { return OnMouseButtonPressed(ev); });
        dispatcher.Dispatch<MouseButtonReleasedEvent>(
            [this](MouseButtonReleasedEvent& ev) { return OnMouseButtonReleased(ev); });
        dispatcher.Dispatch<MouseMovedEvent>(
            [this](MouseMovedEvent& ev) { return OnMouseMoved(ev); });
        dispatcher.Dispatch<KeyReleasedEvent>(
            [this](KeyReleasedEvent& ev) { return OnKeyReleased(ev); });
    }

    void SeascapeLayer::OnAttach() {
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

    void SeascapeLayer::OnDetach() {
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

    void SeascapeLayer::OnUpdate(float dt) {
        m_Time      += dt;
        m_DeltaTime  = dt;
        ++m_Frame;
    }

    void SeascapeLayer::OnRender() {
        if (!m_SeascapeProgram)
            return;

        auto& window = Core::Application::Get().GetWindow();
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

    void SeascapeLayer::OnImGuiRender() {
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

    bool SeascapeLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e) {
        if (e.GetMouseButton() == SDL_BUTTON_LEFT) {
            if (!m_MouseDown) {
                m_MouseClickPos = m_MousePos;
            }
            m_MouseDown = true;
        }
        return false;
    }

    bool SeascapeLayer::OnMouseButtonReleased(MouseButtonReleasedEvent& e) {
        if (e.GetMouseButton() == SDL_BUTTON_LEFT) {
            m_MouseDown = false;
        }
        return false;
    }

    bool SeascapeLayer::OnMouseMoved(MouseMovedEvent& e) {
        m_MousePos = ImVec2((float)e.GetX(), (float)e.GetY());
        return false;
    }

    bool SeascapeLayer::OnKeyReleased(KeyReleasedEvent& e) {
        if (e.GetKeyCode() == SDLK_SPACE) {
            Nova::Core::Application::Get().GetLayerStack().QueueLayerTransition<SpiralGalaxyLayer>(this);
            std::cout << "SeascapeLayer: Transition to SpiralGalaxyLayer requested." << std::endl;
            return true;
        }
        return false;
    }
}