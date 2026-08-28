#ifndef CAMERACONTROLLER_H
#define CAMERACONTROLLER_H

#include <glm/glm.hpp>

#include "Math/Camera.h"
#include "Events/InputEvents.h"

namespace Nova::App {

    class CameraController {
    public:
        bool OnMouseButtonPressed(Nova::Core::Events::MouseButtonPressedEvent& e, bool viewportHovered);
        bool OnMouseButtonReleased(Nova::Core::Events::MouseButtonReleasedEvent& e);
        bool OnMouseMoved(Nova::Core::Events::MouseMovedEvent& e, Nova::Core::Math::Camera& camera);
        bool OnMouseScrolled(Nova::Core::Events::MouseScrolledEvent& e, bool viewportHovered, Nova::Core::Math::Camera& camera);

        void ApplyTo(Nova::Core::Math::Camera& camera);

    private:
        glm::vec3 m_Target{0.0f, 0.0f, 0.0f};
        float m_Distance = 18.0f;

        float m_Yaw = glm::radians(35.0f);
        float m_Pitch = glm::radians(25.0f);

        float m_RotateSensitivity = 0.025f;
        float m_ZoomSensitivity = 0.5f;

        bool m_IsRotating = false;
        bool m_HasLastMousePos = false;
        glm::vec2 m_LastMousePos{0.0f, 0.0f};
    };

} // namespace Nova::App

#endif // CAMERACONTROLLER_H