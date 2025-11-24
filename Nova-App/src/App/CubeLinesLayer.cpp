#include "App/CubeLinesLayer.h"
#include "App/SingularityLayer.h"

#include <iostream>

namespace Nova::App {

    CubeLinesLayer::~CubeLinesLayer() = default;

    void CubeLinesLayer::OnEvent(Event& e) {
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

    void CubeLinesLayer::OnAttach() {
        // Load shader
        std::string novaAppRootDir = NOVA_APP_ROOT_DIR;
        m_CubeLinesProgram = Nova::Core::Renderer::OpenGL::LoadRenderShader(
            novaAppRootDir + "/shaders/OpenGL/cubeLines/cubeLines.vert",
            novaAppRootDir + "/shaders/OpenGL/cubeLines/cubeLines.frag"
        );

        if (!m_CubeLinesProgram) {
            std::cerr << "Failed to load cube lines shader program\n";
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

    void CubeLinesLayer::OnDetach() {
        if (m_CubeLinesProgram) {
            glDeleteProgram(m_CubeLinesProgram);
            m_CubeLinesProgram = 0;
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

    void CubeLinesLayer::OnUpdate(float dt) {
        m_Time      += dt;
        m_DeltaTime  = dt;
        ++m_Frame;
    }

    void CubeLinesLayer::OnRender() {
        if (!m_CubeLinesProgram)
            return;

        auto& window = Core::Application::Get().GetWindow();
        int w, h;
        window.GetWindowSize(w, h);

        glUseProgram(m_CubeLinesProgram);

        // Uniforms Shadertoy
        GLint locResolution = glGetUniformLocation(m_CubeLinesProgram, "iResolution");
        GLint locTime = glGetUniformLocation(m_CubeLinesProgram, "iTime");
        GLint locMouse = glGetUniformLocation(m_CubeLinesProgram, "iMouse");

        if (locResolution >= 0)
            glUniform3f(locResolution, (float)w, (float)h, 1.0f);

        if (locTime >= 0)
            glUniform1f(locTime, m_Time);

        if (locMouse >= 0) {
            ImGuiIO& io = ImGui::GetIO();

            float curX = 0.0f;
            float curY = 0.0f;

            // Si ImGui ne veut PAS la souris et que le bouton gauche est enfonc�,
            // alors on laisse la cam�ra du shader utiliser la souris.
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

    void CubeLinesLayer::OnImGuiRender() {
        ImGui::Begin("Cube lines");

        ImGuiIO& io = ImGui::GetIO();
        float fps = io.Framerate;
        float msPerFrame = fps > 0.0f ? 1000.0f / fps : 0.0f;

        ImGui::Separator();
        ImGui::Text("Frame time: %.3f ms (%.1f FPS)", msPerFrame, fps);

        ImGui::End();
    }

    bool CubeLinesLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e) {
        if (e.GetMouseButton() == SDL_BUTTON_LEFT) {
            if (!m_MouseDown) {
                // On utilise la dernière position connue (mise à jour par MouseMovedEvent)
                m_MouseClickPos = m_MousePos;
            }
            m_MouseDown = true;
        }
        return false;
    }

    bool CubeLinesLayer::OnMouseButtonReleased(MouseButtonReleasedEvent& e) {
        if (e.GetMouseButton() == SDL_BUTTON_LEFT) {
            m_MouseDown = false;
        }
        return false;
    }

    bool CubeLinesLayer::OnMouseMoved(MouseMovedEvent& e) {
        m_MousePos = ImVec2((float)e.GetX(), (float)e.GetY());
        return false;
    }

    bool CubeLinesLayer::OnKeyReleased(KeyReleasedEvent& e) {
        if (e.GetKeyCode() == SDLK_SPACE) {
            Nova::Core::Application::Get().GetLayerStack().QueueLayerTransition<SingularityLayer>(this);
            std::cout << "CubeLinesLayer: Transition to SingularityLayer requested." << std::endl;
            return true;
        }
        return false;
    }
}