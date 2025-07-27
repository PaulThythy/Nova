#include <glm/gtc/constants.hpp>

#include "Renderer/OpenGL/Graphics/Camera.hpp"

namespace Nova {
    namespace Renderer {
        namespace OpenGL {
            namespace Graphics {

                void Camera::init() {
                    glGenBuffers(1, &m_UBO);
                    glBindBuffer(GL_UNIFORM_BUFFER, m_UBO);
                    glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);
                    glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_UBO);
                    glBindBuffer(GL_UNIFORM_BUFFER, 0);
                }

                void Camera::update() {
                    // compute projection
                    float fov = glm::radians(m_Node->m_Fov);
                    //TODO aspect ratio dynamic ? maybe put it in CameraNode struct
                    glm::mat4 projection = glm::perspective(fov, 16.0f / 9.0f, m_Node->m_NearPlane, m_Node->m_FarPlane);
                    glm::vec3 position = m_Node->m_Position;
                    glm::vec3 target = m_Node->m_LookAt;
                    glm::vec3 up = m_Node->m_Up;

                    glm::mat4 view = glm::lookAt(position, target, up);

                    glBindBuffer(GL_UNIFORM_BUFFER, m_UBO);
                    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &view);
                    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), &projection);
                    glBindBuffer(GL_UNIFORM_BUFFER, 0);
                }

                void Camera::destroy() {
                    if (m_UBO) {
                        glDeleteBuffers(1, &m_UBO);
                        m_UBO = 0;
                    }
                }

            } // namespace Graphics
        } // namespace OpenGL
    } // namespace Renderer
} // namespace Nova