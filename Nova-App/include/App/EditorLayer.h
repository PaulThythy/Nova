#ifndef EDITORLAYER_H
#define EDITORLAYER_H

#include <iostream>
#include <vector>
#include <glad/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Events/Event.h"
#include "Events/InputEvents.h"

#include "Core/Layer.h"
#include "Core/Application.h"

#include "App/AppLayer.h"

using namespace Nova::Core::Events;
using namespace Nova::Core;

namespace Nova::App {

    class EditorLayer : public Layer {
    public:
        explicit EditorLayer(): Layer("EditorLayer") {}
        ~EditorLayer() override;

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(float dt) override;
        void OnRender() override;
        void OnImGuiRender() override;
        void OnEvent(Event& e) override;

        void SetViewportSize(float width, float height);
        GLuint GetViewportTexture() const { return m_ColorAttachment; }

        struct Line {
            GLuint m_LineProgram;
            GLuint m_VBO, m_VAO;
            std::vector<float> m_Vertices;
            glm::vec3 m_StartPoint;
            glm::vec3 m_EndPoint;
            glm::mat4 m_MVP;
            glm::vec3 m_Color;

            Line(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color): m_StartPoint(start), m_EndPoint(end), m_Color(color) {
                m_MVP = glm::mat4(1.0f);

                const char *vertexShaderSource = "#version 330 core\n"
                    "layout (location = 0) in vec3 aPos;\n"
                    "uniform mat4 u_MVP;\n"
                    "void main()\n"
                    "{\n"
                    "   gl_Position = u_MVP * vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
                    "}\0";
                const char *fragmentShaderSource = "#version 330 core\n"
                    "out vec4 FragColor;\n"
                    "uniform vec3 u_Color;\n"
                    "void main()\n"
                    "{\n"
                    "   FragColor = vec4(u_Color, 1.0f);\n"
                    "}\n\0";

                // vertex shader
                int vertexShader = glCreateShader(GL_VERTEX_SHADER);
                glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
                glCompileShader(vertexShader);
                // check for shader compile errors

                // fragment shader
                int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
                glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
                glCompileShader(fragmentShader);
                // check for shader compile errors

                // link shaders
                m_LineProgram = glCreateProgram();
                glAttachShader(m_LineProgram, vertexShader);
                glAttachShader(m_LineProgram, fragmentShader);
                glLinkProgram(m_LineProgram);
                // check for linking errors

                glDeleteShader(vertexShader);
                glDeleteShader(fragmentShader);

                m_Vertices = {
                    start.x, start.y, start.z,
                    end.x, end.y, end.z,

               };

                glGenVertexArrays(1, &m_VAO);
                glGenBuffers(1, &m_VBO);
                glBindVertexArray(m_VAO);

                glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
                glBufferData(GL_ARRAY_BUFFER,
                    m_Vertices.size() * sizeof(float),
                        m_Vertices.data(),
                        GL_STATIC_DRAW
                );

                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(0);

                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBindVertexArray(0);
            }

            void draw() {
                glUseProgram(m_LineProgram);
                glUniformMatrix4fv(glGetUniformLocation(m_LineProgram, "u_MVP"), 1, GL_FALSE, &m_MVP[0][0]);
                glUniform3fv(glGetUniformLocation(m_LineProgram, "u_Color"), 1, &m_Color[0]);

                glBindVertexArray(m_VAO);
                glDrawArrays(GL_LINES, 0, 2);
            }

            ~Line() {
                glDeleteVertexArrays(1, &m_VAO);
                glDeleteBuffers(1, &m_VBO);
                glDeleteProgram(m_LineProgram);
            }
        };

        struct Grid {
            GLuint m_GridProgram;
            GLuint m_VBO, m_VAO;
            std::vector<float> m_Vertices;
            glm::mat4 m_MVP;
            glm::vec3 m_Color;

            Grid(int halfSize, float spacing, const glm::vec3& color): m_Color(color) {
                const char* vertexShaderSource = "#version 330 core\n"
                    "layout (location = 0) in vec3 aPos;\n"
                    "uniform mat4 u_MVP;\n"
                    "void main()\n"
                    "{\n"
                    "   gl_Position = u_MVP * vec4(aPos, 1.0);\n"
                    "}\0";

                const char* fragmentShaderSource = "#version 330 core\n"
                    "out vec4 FragColor;\n"
                    "uniform vec3 u_Color;\n"
                    "void main()\n"
                    "{\n"
                    "   FragColor = vec4(u_Color, 1.0);\n"
                    "}\n\0";

                // vertex shader
                GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
                glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
                glCompileShader(vertexShader);

                GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
                glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
                glCompileShader(fragmentShader);

                // link shaders
                m_GridProgram = glCreateProgram();
                glAttachShader(m_GridProgram, vertexShader);
                glAttachShader(m_GridProgram, fragmentShader);
                glLinkProgram(m_GridProgram);

                glDeleteShader(vertexShader);
                glDeleteShader(fragmentShader);

                float extent = halfSize * spacing;
                for (int i = -halfSize; i <= halfSize; ++i) {
                    float z = i * spacing;
                    m_Vertices.push_back(-extent); m_Vertices.push_back(0.0f); m_Vertices.push_back(z);
                    m_Vertices.push_back(+extent); m_Vertices.push_back(0.0f); m_Vertices.push_back(z);
                }
                for (int i = -halfSize; i <= halfSize; ++i) {
                    float x = i * spacing;
                    m_Vertices.push_back(x); m_Vertices.push_back(0.0f); m_Vertices.push_back(-extent);
                    m_Vertices.push_back(x); m_Vertices.push_back(0.0f); m_Vertices.push_back(+extent);
                }
                glGenVertexArrays(1, &m_VAO);
                glGenBuffers(1, &m_VBO);
                glBindVertexArray(m_VAO);

                glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
                glBufferData(GL_ARRAY_BUFFER,
                    m_Vertices.size() * sizeof(float),
                    m_Vertices.data(),
                    GL_STATIC_DRAW);

                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(0);

                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBindVertexArray(0);
            }

            void draw() {
                glUseProgram(m_GridProgram);
                glUniformMatrix4fv(glGetUniformLocation(m_GridProgram, "u_MVP"), 1, GL_FALSE, &m_MVP[0][0]);
                glUniform3fv(glGetUniformLocation(m_GridProgram, "u_Color"), 1, &m_Color[0]);

                glBindVertexArray(m_VAO);
                GLsizei vertexCount = static_cast<GLsizei>(m_Vertices.size() / 3);
                glDrawArrays(GL_LINES, 0, vertexCount);
                glBindVertexArray(0);
            }

            ~Grid() {
                if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
                if (m_VBO) glDeleteBuffers(1, &m_VBO);
                if (m_GridProgram) glDeleteProgram(m_GridProgram);
            }
        };

    private:
        bool OnKeyReleased(KeyReleasedEvent& e);

        std::unique_ptr<Line> m_XAxis;
        std::unique_ptr<Line> m_YAxis;
        std::unique_ptr<Line> m_ZAxis;

        std::unique_ptr<Grid> m_Grid;

        GLuint m_SceneProgram{ 0 };

        glm::vec2 m_ViewportSize{ 0.0f, 0.0f };
        GLuint    m_Framebuffer{ 0 };
        GLuint    m_ColorAttachment{ 0 };
        GLuint    m_DepthAttachment{ 0 };

        void InvalidateFramebuffer();
        void ReleaseFramebuffer();
    };

} // namespace Nova::App

#endif // EDITORLAYER_H