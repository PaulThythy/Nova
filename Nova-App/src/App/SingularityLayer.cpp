#include "App/SingularityLayer.h"
#include "App/SeascapeLayer.h"

#include <iostream>

namespace Nova::App {

    SingularityLayer::~SingularityLayer() = default;

    void SingularityLayer::OnEvent(Event& e) {
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

    void SingularityLayer::OnAttach() {
        // Load shader
        std::string novaAppRootDir = NOVA_APP_ROOT_DIR;
        m_SingularityProgram = Nova::Renderer::OpenGL::LoadRenderShader(
            novaAppRootDir + "/shaders/OpenGL/singularity/singularity.vert",
            novaAppRootDir + "/shaders/OpenGL/singularity/singularity.frag"
        );

        if (!m_SingularityProgram) {
            std::cerr << "Failed to load singularity shader program\n";
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

    void SingularityLayer::OnDetach() {
        if (m_SingularityProgram) {
            glDeleteProgram(m_SingularityProgram);
            m_SingularityProgram = 0;
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

    void SingularityLayer::OnUpdate(float dt) {
        m_Time      += dt;
        m_DeltaTime  = dt;
        ++m_Frame;
    }

    void SingularityLayer::OnRender() {
        if (!m_SingularityProgram)
            return;

        auto& window = Core::Application::Get().GetWindow();
        int w, h;
        window.GetWindowSize(w, h);

        glUseProgram(m_SingularityProgram);

        // Uniforms Shadertoy
        GLint locResolution = glGetUniformLocation(m_SingularityProgram, "iResolution");
        GLint locTime = glGetUniformLocation(m_SingularityProgram, "iTime");
        GLint locMouse = glGetUniformLocation(m_SingularityProgram, "iMouse");

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

    void SingularityLayer::OnImGuiRender() {
        ImGui::Begin("Singularity");

        ImGuiIO& io = ImGui::GetIO();
        float fps = io.Framerate;
        float msPerFrame = fps > 0.0f ? 1000.0f / fps : 0.0f;

        ImGui::Separator();
        ImGui::Text("Frame time: %.3f ms (%.1f FPS)", msPerFrame, fps);

        ImGui::End();
    }

    bool SingularityLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e) {
        if (e.GetMouseButton() == SDL_BUTTON_LEFT) {
            if (!m_MouseDown) {
                m_MouseClickPos = m_MousePos;
            }
            m_MouseDown = true;
        }
        return false;
    }

    bool SingularityLayer::OnMouseButtonReleased(MouseButtonReleasedEvent& e) {
        if (e.GetMouseButton() == SDL_BUTTON_LEFT) {
            m_MouseDown = false;
        }
        return false;
    }

    bool SingularityLayer::OnMouseMoved(MouseMovedEvent& e) {
        m_MousePos = ImVec2((float)e.GetX(), (float)e.GetY());
        return false;
    }

    bool SingularityLayer::OnKeyReleased(KeyReleasedEvent& e) {
        if (e.GetKeyCode() == SDLK_SPACE) {
            Nova::Core::Application::Get().GetLayerStack().QueueLayerTransition<SeascapeLayer>(this);
            std::cout << "SingularityLayer: Transition to SeascapeLayer requested." << std::endl;
            return true;
        }
        return false;
    }
}