#ifndef CAMERACONTROLLER_H
#define CAMERACONTROLLER_H

#include <glm/glm.hpp>

#include "Events/InputEvents.h"
#include "Math/Camera.h"

namespace Nova::App {

    enum class CameraMode {
        Navigation = 0,
        Orbit
    };

    class CameraController {
    public:
        bool OnMouseButtonPressed(Nova::Core::Events::MouseButtonPressedEvent& e, bool viewportHovered);
        bool OnMouseButtonReleased(Nova::Core::Events::MouseButtonReleasedEvent& e);
        bool OnMouseMoved(Nova::Core::Events::MouseMovedEvent& e, Nova::Core::Math::Camera& camera);
        bool OnMouseScrolled(Nova::Core::Events::MouseScrolledEvent& e, bool viewportHovered, Nova::Core::Math::Camera& camera);

        void Update(float dt, Nova::Core::Math::Camera& camera);

        CameraMode GetMode() const { return m_Mode; }

        /** Capture navigation state from the current camera (default mode on startup). */
        void SetNavigationFromCamera(const Nova::Core::Math::Camera& camera);

        /** Orbit around AABB center; animates from the current view. */
        void BeginOrbitFocus(const glm::vec3& target, const glm::vec3& extents, Nova::Core::Math::Camera& camera);

        /** Leave orbit/focus and restore free navigation from the current camera pose. */
        void ExitOrbitMode(Nova::Core::Math::Camera& camera);

    private:
        struct OrbitState {
            glm::vec3 m_Target{0.0f};
            float m_Distance = 18.0f;
            float m_Yaw = 0.0f;
            float m_Pitch = 0.0f;
        };

        struct NavigationState {
            glm::vec3 m_Position{0.0f};
            float m_Yaw = 0.0f;
            float m_Pitch = 0.0f;
        };

        void ApplyOrbit(Nova::Core::Math::Camera& camera);
        void ApplyNavigation(Nova::Core::Math::Camera& camera);
        void SyncOrbitFromCamera(const Nova::Core::Math::Camera& camera);
        static glm::vec3 ForwardFromAngles(float yaw, float pitch);
        static void AnglesFromForward(const glm::vec3& forward, float& yaw, float& pitch);
        static float OrbitDistanceForExtents(const glm::vec3& extents, float fovDegrees);

        CameraMode m_Mode = CameraMode::Navigation;

        OrbitState m_Orbit{};
        NavigationState m_Navigation{};

        float m_RotateSensitivity = 0.025f;
        float m_OrbitZoomSensitivity = 0.5f;
        float m_NavigationMoveSensitivity = 0.35f;

        bool m_IsRotating = false;
        bool m_HasLastMousePos = false;
        glm::vec2 m_LastMousePos{0.0f, 0.0f};

        bool m_FocusAnimating = false;
        float m_FocusAnimDuration = 0.45f;
        float m_FocusAnimElapsed = 0.0f;
        OrbitState m_FocusAnimStart{};
        OrbitState m_FocusAnimEnd{};
    };

} // namespace Nova::App

#endif // CAMERACONTROLLER_H