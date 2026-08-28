#include "App/CameraController.h"

#include <algorithm>
#include <cmath>

#include <SDL3/SDL.h>

namespace Nova::App {

    glm::vec3 CameraController::ForwardFromAngles(float yaw, float pitch) {
        const float cp = std::cos(pitch);
        return glm::normalize(glm::vec3{
            cp * std::sin(yaw),
            std::sin(pitch),
            cp * std::cos(yaw),
        });
    }

    void CameraController::AnglesFromForward(const glm::vec3& forward, float& yaw, float& pitch) {
        const glm::vec3 dir = glm::normalize(forward);
        pitch = std::asin(std::clamp(dir.y, -1.0f, 1.0f));
        yaw = std::atan2(dir.x, dir.z);
    }

    float CameraController::OrbitDistanceForExtents(const glm::vec3& extents, float fovDegrees) {
        const float radius = glm::length(extents);
        const float fovRad = glm::radians(fovDegrees * 0.5f);
        const float sinFov = std::max(std::sin(fovRad), 1e-3f);
        return std::max(radius / sinFov * 1.35f, 0.5f);
    }

    void CameraController::SetNavigationFromCamera(const Nova::Core::Math::Camera& camera) {
        m_Mode = CameraMode::Navigation;
        m_FocusAnimating = false;
        m_IsRotating = false;

        m_Navigation.m_Position = camera.m_LookFrom;
        AnglesFromForward(camera.m_LookAt - camera.m_LookFrom, m_Navigation.m_Yaw, m_Navigation.m_Pitch);
    }

    void CameraController::SyncOrbitFromCamera(const Nova::Core::Math::Camera& camera) {
        const glm::vec3 offset = camera.m_LookFrom - camera.m_LookAt;
        m_Orbit.m_Target = camera.m_LookAt;
        m_Orbit.m_Distance = std::max(glm::length(offset), 0.2f);
        if (m_Orbit.m_Distance > 1e-4f) {
            const glm::vec3 dir = offset / m_Orbit.m_Distance;
            AnglesFromForward(dir, m_Orbit.m_Yaw, m_Orbit.m_Pitch);
        }
    }

    void CameraController::BeginOrbitFocus(const glm::vec3& target, const glm::vec3& extents, Nova::Core::Math::Camera& camera) {
        m_Mode = CameraMode::Orbit;
        m_IsRotating = false;

        OrbitState start{};
        SyncOrbitFromCamera(camera);
        start = m_Orbit;

        OrbitState end{};
        end.m_Target = target;
        end.m_Distance = OrbitDistanceForExtents(extents, camera.m_FOV);
        end.m_Yaw = start.m_Yaw;
        end.m_Pitch = start.m_Pitch;

        const float maxPitch = glm::radians(89.0f);
        end.m_Pitch = std::clamp(end.m_Pitch, -maxPitch, maxPitch);

        m_FocusAnimStart = start;
        m_FocusAnimEnd = end;
        m_FocusAnimElapsed = 0.0f;
        m_FocusAnimating = true;

        m_Orbit = start;
        ApplyOrbit(camera);
    }

    void CameraController::ExitOrbitMode(Nova::Core::Math::Camera& camera) {
        m_FocusAnimating = false;
        m_IsRotating = false;
        SetNavigationFromCamera(camera);
    }

    void CameraController::Update(float dt, Nova::Core::Math::Camera& camera) {
        if (!m_FocusAnimating)
            return;

        m_FocusAnimElapsed += dt;
        const float t = std::clamp(m_FocusAnimElapsed / m_FocusAnimDuration, 0.0f, 1.0f);
        const float smooth = t * t * (3.0f - 2.0f * t);

        m_Orbit.m_Target = glm::mix(m_FocusAnimStart.m_Target, m_FocusAnimEnd.m_Target, smooth);
        m_Orbit.m_Distance = glm::mix(m_FocusAnimStart.m_Distance, m_FocusAnimEnd.m_Distance, smooth);
        m_Orbit.m_Yaw = glm::mix(m_FocusAnimStart.m_Yaw, m_FocusAnimEnd.m_Yaw, smooth);
        m_Orbit.m_Pitch = glm::mix(m_FocusAnimStart.m_Pitch, m_FocusAnimEnd.m_Pitch, smooth);

        ApplyOrbit(camera);

        if (t >= 1.0f)
            m_FocusAnimating = false;
    }

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

        if (m_Mode == CameraMode::Orbit) {
            m_Orbit.m_Yaw   -= delta.x * m_RotateSensitivity;
            m_Orbit.m_Pitch += delta.y * m_RotateSensitivity;
            ApplyOrbit(camera);
        } else {
            m_Navigation.m_Yaw   -= delta.x * m_RotateSensitivity;
            m_Navigation.m_Pitch -= delta.y * m_RotateSensitivity;
            ApplyNavigation(camera);
        }
        return true;
    }

    bool CameraController::OnMouseScrolled(Nova::Core::Events::MouseScrolledEvent& e, bool viewportHovered, Nova::Core::Math::Camera& camera) {
        if (!viewportHovered)
            return false;

        if (m_Mode == CameraMode::Orbit) {
            m_Orbit.m_Distance -= static_cast<float>(e.GetYOffset()) * m_OrbitZoomSensitivity;
            ApplyOrbit(camera);
        } else {
            const glm::vec3 forward = ForwardFromAngles(m_Navigation.m_Yaw, m_Navigation.m_Pitch);
            m_Navigation.m_Position += forward * (static_cast<float>(e.GetYOffset()) * m_NavigationMoveSensitivity);
            ApplyNavigation(camera);
        }
        return true;
    }

    void CameraController::ApplyOrbit(Nova::Core::Math::Camera& camera) {
        const float maxPitch = glm::radians(89.0f);
        m_Orbit.m_Pitch = std::clamp(m_Orbit.m_Pitch, -maxPitch, maxPitch);
        m_Orbit.m_Distance = std::max(0.2f, m_Orbit.m_Distance);

        const glm::vec3 offset = ForwardFromAngles(m_Orbit.m_Yaw, m_Orbit.m_Pitch) * m_Orbit.m_Distance;
        camera.m_LookAt = m_Orbit.m_Target;
        camera.m_LookFrom = m_Orbit.m_Target + offset;
        camera.m_Up = {0.0f, 1.0f, 0.0f};
    }

    void CameraController::ApplyNavigation(Nova::Core::Math::Camera& camera) {
        const float maxPitch = glm::radians(89.0f);
        m_Navigation.m_Pitch = std::clamp(m_Navigation.m_Pitch, -maxPitch, maxPitch);

        const glm::vec3 forward = ForwardFromAngles(m_Navigation.m_Yaw, m_Navigation.m_Pitch);
        camera.m_LookFrom = m_Navigation.m_Position;
        camera.m_LookAt = m_Navigation.m_Position + forward;
        camera.m_Up = {0.0f, 1.0f, 0.0f};
    }

} // namespace Nova::App