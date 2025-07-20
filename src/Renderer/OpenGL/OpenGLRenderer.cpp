#include <iostream>

#include "Renderer/OpenGL/OpenGLRenderer.hpp"

static const char* vertexShaderSrc = R"(
#version 130
in vec2 position;
void main() {
    gl_Position = vec4(position, 0.0, 1.0);
}
)";
static const char* fragmentShaderSrc = R"(
#version 130
out vec4 color;
void main() {
    color = vec4(1.0, 0.0, 0.0, 1.0);
}
)";

namespace Nova {
    namespace Renderer {
        namespace OpenGL {

            void OpenGLRenderer::init(int width, int height) {
                m_ViewportWidth = width;
                m_ViewportHeight = height;

                if(glewInit() != GLEW_OK) {
                    std::cerr << "Failed to initialize GLEW\n";
                }

                float vertices[] = {
                    -0.5f, -0.5f,
                     0.5f, -0.5f,
                     0.0f,  0.5f
                };

                glGenVertexArrays(1, &m_VAO);
                glGenBuffers(1, &m_VBO);

                glBindVertexArray(m_VAO);
                glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

                GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
                glShaderSource(vertShader, 1, &vertexShaderSrc, nullptr);
                glCompileShader(vertShader);

                GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
                glShaderSource(fragShader, 1, &fragmentShaderSrc, nullptr);
                glCompileShader(fragShader);

                m_shaderProgram = glCreateProgram();
                glAttachShader(m_shaderProgram, vertShader);
                glAttachShader(m_shaderProgram, fragShader);
                glLinkProgram(m_shaderProgram);

                glDeleteShader(vertShader);
                glDeleteShader(fragShader);

                GLint posAttrib = glGetAttribLocation(m_shaderProgram, "position");
                glEnableVertexAttribArray(posAttrib);
                glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);

                glBindVertexArray(0);

                initFBO(width, height);
            }

            void OpenGLRenderer::initFBO(int width, int height) {
                if (m_FBO) {
                    glDeleteFramebuffers(1, &m_FBO);
                    glDeleteTextures(1, &m_ColorTexture);
                    glDeleteRenderbuffers(1, &m_DepthBuffer);
                }

                glGenFramebuffers(1, &m_FBO);
                glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

                glGenTextures(1, &m_ColorTexture);
                glBindTexture(GL_TEXTURE_2D, m_ColorTexture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorTexture, 0);

                glGenRenderbuffers(1, &m_DepthBuffer);
                glBindRenderbuffer(GL_RENDERBUFFER, m_DepthBuffer);
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthBuffer);

                if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                    std::cerr << "FBO not complete!\n";

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }

            void OpenGLRenderer::onResize(int width, int height) {
                m_ViewportWidth  = width;
                m_ViewportHeight = height;
                initFBO(width, height);
            }


            void OpenGLRenderer::render() {
                glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
                glViewport(0, 0, m_ViewportWidth, m_ViewportHeight);
                glEnable(GL_DEPTH_TEST);
                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                glUseProgram(m_shaderProgram);
                glBindVertexArray(m_VAO);
                glDrawArrays(GL_TRIANGLES, 0, 3);
                glBindVertexArray(0);
                glUseProgram(0);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }

            void OpenGLRenderer::destroy() {
                glDeleteVertexArrays(1, &m_VAO);
                glDeleteBuffers(1, &m_VBO);
                glDeleteProgram(m_shaderProgram);
            }

        } // namespace OpenGL
    } // namespace Renderer
} // namespace Nova
