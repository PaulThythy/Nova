#ifndef CAMERA_COMPONENT_HPP
#define CAMERA_COMPONENT_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Nova::Components {

    struct CameraComponent {
        glm::vec3 m_LookFrom = {0.0f, 2.0f, 5.0f};
        glm::vec3 m_LookAt = {0.0f, 0.0f, 0.0f};
        glm::vec3 m_Up = {0.0f, 1.0f, 0.0f};
        float m_Fov = 45.0f;
        float m_AspectRatio = 16.0f / 9.0f;
        float m_NearPlane = 0.1f;
        float m_FarPlane = 100.0f;

        bool m_IsViewportCamera = false;
        bool m_IsRenderingCamera = false;

        CameraComponent() = default;
        ~CameraComponent() = default;

        glm::mat4 getViewMatrix() const {
            return glm::lookAt(m_LookFrom, m_LookAt, m_Up);
        }

        glm::mat4 getProjectionMatrix() const {
            return glm::perspective(glm::radians(m_Fov), m_AspectRatio, m_NearPlane, m_FarPlane);
        }
    };

} // namespace Nova::Components

#endif // CAMERA_COMPONENT_HPP