#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Renderer/OpenGL/OpenGLRenderer.hpp"
#include "Renderer/OpenGL/Shader.hpp"

namespace Nova {
    namespace Renderer {
        namespace OpenGL {

            void OpenGLRenderer::init(const Nova::Scene::Scene& scene) {
                m_Scene = &scene;

                if(glewInit() != GLEW_OK) {
                    std::cerr << "Failed to initialize GLEW\n";
                }

                std::string vertexPath = std::string(SHADER_DIR) + "/vertex.vert";
                std::string fragmentPath = std::string(SHADER_DIR) + "/fragment.frag";
                m_shaderProgram = loadRenderShader(vertexPath, fragmentPath);

                if (m_shaderProgram == 0) {
                    std::cerr << "Failed to load or compile shaders!" << std::endl;
                }

                initFBO(m_ViewportWidth, m_ViewportHeight);
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

            void OpenGLRenderer::updateViewportSize(int width, int height) {
                if (width != m_ViewportWidth || height != m_ViewportHeight) {
                    m_ViewportWidth = width;
                    m_ViewportHeight = height;
                    initFBO(width, height);

                    if (m_Scene && m_Scene->m_ViewportCamera) {
                        m_Scene->m_ViewportCamera->m_AspectRatio = static_cast<float>(width) / static_cast<float>(height);
                    }
                }
            }

            void OpenGLRenderer::uploadSphereToGPU(const Nova::Scene::Sphere* sphere, GLuint& vao, GLuint& vbo, GLuint& ibo) {
                glGenVertexArrays(1, &vao);
                glGenBuffers(1, &vbo);
                glGenBuffers(1, &ibo);

                glBindVertexArray(vao);

                struct Vertex {
                    glm::vec3 position;
                    glm::vec3 normal;
                };
                std::vector<Vertex> vertices;
                vertices.reserve(sphere->m_Vertices.size());
                for (size_t i = 0; i < sphere->m_Vertices.size(); ++i) {
                    vertices.push_back({ sphere->m_Vertices[i], sphere->m_Normals[i] });
                }

                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
                glEnableVertexAttribArray(0);

                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
                glEnableVertexAttribArray(1);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphere->m_Indices.size() * sizeof(unsigned int), sphere->m_Indices.data(), GL_STATIC_DRAW);

                glBindVertexArray(0);
            }

            void OpenGLRenderer::uploadPlaneToGPU(const Nova::Scene::Plane* plane, GLuint& vao, GLuint& vbo, GLuint& ibo) {
                glGenVertexArrays(1, &vao);
                glGenBuffers(1, &vbo);
                glGenBuffers(1, &ibo);

                glBindVertexArray(vao);

                struct Vertex {
                    glm::vec3 position;
                    glm::vec3 normal;
                };
                std::vector<Vertex> vertices;
                vertices.reserve(plane->m_Vertices.size());
                for (size_t i = 0; i < plane->m_Vertices.size(); ++i) {
                    vertices.push_back({ plane->m_Vertices[i], plane->m_Normals[i] });
                }

                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
                glEnableVertexAttribArray(0);

                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
                glEnableVertexAttribArray(1);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, plane->m_Indices.size() * sizeof(unsigned int), plane->m_Indices.data(), GL_STATIC_DRAW);

                glBindVertexArray(0);
            }


            void OpenGLRenderer::uploadLightToShader(const Nova::Scene::Light* light, GLuint shaderProgram) {
                GLint locLightPos = glGetUniformLocation(shaderProgram, "u_LightPosition");
                GLint locLightColor = glGetUniformLocation(shaderProgram, "u_LightColor");
                GLint locLightIntensity = glGetUniformLocation(shaderProgram, "u_LightIntensity");

                glm::vec3 worldPos = glm::vec3(light->getModelMatrix() * glm::vec4(0, 0, 0, 1));

                glUniform3fv(locLightPos, 1, glm::value_ptr(worldPos));
                glUniform3fv(locLightColor, 1, glm::value_ptr(light->m_Color));
                glUniform1f(locLightIntensity, light->m_Intensity);
            }

            void OpenGLRenderer::render() {
                glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
                glViewport(0, 0, m_ViewportWidth, m_ViewportHeight);
                glEnable(GL_DEPTH_TEST);
                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                glUseProgram(m_shaderProgram);
                glm::mat4 view = m_Scene->m_ViewportCamera->getViewMatrix();
                glm::mat4 projection = m_Scene->m_ViewportCamera->getProjectionMatrix();

                GLint locView = glGetUniformLocation(m_shaderProgram, "u_View");
                GLint locProj = glGetUniformLocation(m_shaderProgram, "u_Projection");
                glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
                glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));

                glUniform3fv(glGetUniformLocation(m_shaderProgram, "u_CameraPos"), 1, glm::value_ptr(m_Scene->m_ViewportCamera->m_Position));
                glUniform3f(glGetUniformLocation(m_shaderProgram, "u_ObjectColor"), 1.0f, 0.0f, 0.0f);


                for (auto* node : m_Scene->m_Roots) {
                    if (auto* light = dynamic_cast<Nova::Scene::Light*>(node)) {
                        uploadLightToShader(light, m_shaderProgram);
                    }

                    if (auto* sphere = dynamic_cast<Nova::Scene::Sphere*>(node)) {
                        if (sphere->m_Vertices.empty() || sphere->m_Indices.empty()) {
                            sphere->init();
                        }

                        GLuint vao, vbo, ibo;
                        uploadSphereToGPU(sphere, vao, vbo, ibo);

                        glm::mat4 model = sphere->getModelMatrix();
                        GLint locModel = glGetUniformLocation(m_shaderProgram, "u_Model");
                        glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));

                        glBindVertexArray(vao);
                        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(sphere->m_Indices.size()), GL_UNSIGNED_INT, 0);
                        glBindVertexArray(0);

                        glDeleteVertexArrays(1, &vao);
                        glDeleteBuffers(1, &vbo);
                        glDeleteBuffers(1, &ibo);
                    }

                    else if (auto* plane = dynamic_cast<Nova::Scene::Plane*>(node)) {
                        if (plane->m_Vertices.empty() || plane->m_Indices.empty()) {
                            plane->init();
                        }

                        GLuint vao, vbo, ibo;
                        uploadPlaneToGPU(plane, vao, vbo, ibo);

                        glm::mat4 model = plane->getModelMatrix();
                        GLint locModel = glGetUniformLocation(m_shaderProgram, "u_Model");
                        glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));

                        glBindVertexArray(vao);
                        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(plane->m_Indices.size()), GL_UNSIGNED_INT, 0);
                        glBindVertexArray(0);

                        glDeleteVertexArrays(1, &vao);
                        glDeleteBuffers(1, &vbo);
                        glDeleteBuffers(1, &ibo);
                    }
                }

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
