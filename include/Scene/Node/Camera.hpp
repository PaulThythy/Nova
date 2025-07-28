#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Scene/Node/Node.hpp"

using namespace Nova::Scene;

namespace Nova {
    namespace Scene {

        struct Camera : public Node {
            glm::vec3 m_Position = {0.0f, 0.0f, 5.0f};
            glm::vec3 m_LookAt = {0.0f, 0.0f, 0.0f};
            glm::vec3 m_Up = {0.0f, 1.0f, 0.0f};
            float m_Fov = 45.0f;
            float m_AspectRatio = 16.0f / 9.0f;
            float m_NearPlane = 0.1f;
            float m_FarPlane = 100.0f;

            Camera(const std::string& name) : Node(name) {}

            glm::mat4 getViewMatrix() const {
                return glm::lookAt(m_Position, m_LookAt, m_Up);
            }

            glm::mat4 getProjectionMatrix() const {
                return glm::perspective(glm::radians(m_Fov), m_AspectRatio, m_NearPlane, m_FarPlane);
            }
        };

    } // namespace Scene
} // namespace Nova

#endif // CAMERA_HPP