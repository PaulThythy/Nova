#include "App/CameraController.h"

#include <algorithm>
#include <cmath>

#include <SDL3/SDL.h>

namespace Nova::App {

    bool CameraController::OnMouseButtonPressed(Nova::Core::Events::MouseButtonPressedEvent& e, bool viewportHovered) {
        if (!viewportHovered)
            return false;

        if (e.GetMouseButton() == SDL_BUTTON_RIGHT) {
            m_IsRotating = true;
            m_HasLastMousePos = false;
            return true;
        }
        return false;
    }

    bool CameraController::OnMouseButtonReleased(Nova::Core::Events::MouseButtonReleasedEvent& e) {
        if (e.GetMouseButton() == SDL_BUTTON_RIGHT) {
            m_IsRotating = false;
            return true;
        }
        return false;
    }

    bool CameraController::OnMouseMoved(Nova::Core::Events::MouseMovedEvent& e, Nova::Core::Math::Camera& camera) {
        const glm::vec2 mousePos{ e.GetX(), e.GetY() };

        if (!m_IsRotating) {
            m_LastMousePos = mousePos;
            m_HasLastMousePos = true;
            return false;
        }

        if (!m_HasLastMousePos) {
            m_LastMousePos = mousePos;
            m_HasLastMousePos = true;
            return true;
        }

        const glm::vec2 delta = mousePos - m_LastMousePos;
        m_LastMousePos = mousePos;

        m_Yaw   -= delta.x * m_RotateSensitivity;
        m_Pitch += delta.y * m_RotateSensitivity;

        ApplyTo(camera);
        return true;
    }

    bool CameraController::OnMouseScrolled(Nova::Core::Events::MouseScrolledEvent& e, bool viewportHovered, Nova::Core::Math::Camera& camera) {
        if (!viewportHovered)
            return false;

        m_Distance -= e.GetYOffset() * m_ZoomSensitivity;
        ApplyTo(camera);
        return true;
    }

    void CameraController::ApplyTo(Nova::Core::Math::Camera& camera) {
        const float maxPitch = glm::radians(89.0f);
        m_Pitch = std::clamp(m_Pitch, -maxPitch, maxPitch);
        m_Distance = std::max(0.2f, m_Distance);

        const float cp = std::cos(m_Pitch);

        glm::vec3 offset;
        offset.x = m_Distance * cp * std::sin(m_Yaw);
        offset.y = m_Distance * std::sin(m_Pitch);
        offset.z = m_Distance * cp * std::cos(m_Yaw);

        camera.m_LookAt = m_Target;
        camera.m_LookFrom = m_Target + offset;
        camera.m_Up = {0.0f, 1.0f, 0.0f};
    }

} // namespace Nova::App