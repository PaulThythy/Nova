#include "GUI/Panels/ViewportPanel.hpp"
#include "Renderer/Renderer.hpp"
#include "Math/Ray.hpp"
#include "Math/AABB.hpp"
#include "Components/TransformComponent.hpp"
#include "Components/MeshComponent.hpp"

#include <iostream>

namespace Nova::GUI::ViewportPanel {

    void Render(Nova::Renderer::IRenderer* renderer, Nova::Scene& scene) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImVec2 size = ImGui::GetContentRegionAvail();
        renderer->updateViewportSize((int)size.x, (int)size.y);

        ImGui::Image((ImTextureID)renderer->getImGuiTextureID(), size, ImVec2(0, 1), ImVec2(1, 0));

        Controls(scene);

        DeleteSelection(scene);

        ClearSelection(scene);

        Picking(scene, size);

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void Controls(Nova::Scene& scene) {
        // Tunables (per second / per pixel)
        constexpr float ROTATE_SPEED = 0.002f;      // radians per pixel per second
        constexpr float PAN_SPEED    = 0.1f;        // world units per pixel per second (scaled by distance)
        constexpr float MOVE_SPEED   = 5.0f;        // world units per second (ZQSD)

        static ImVec2 lastMousePos{ 0,0 };

        auto camE = scene.getViewportCamera();
        Nova::Components::CameraComponent* camPtr = nullptr;
        if (camE != entt::null)
            camPtr = &scene.registry().get<Nova::Components::CameraComponent>(camE);
        if (!camPtr) return;

        ImGuiIO& io = ImGui::GetIO();
        const float dt = (io.DeltaTime > 0.f ? io.DeltaTime : 1.f/60.f);

        // Mouse delta (in pixels) this frame
        ImVec2 mousePos = io.MousePos;
        ImVec2 delta = { mousePos.x - lastMousePos.x, mousePos.y - lastMousePos.y };
        lastMousePos = mousePos;

        // We only act when the viewport window is both focused and hovered.
        // (Do NOT rely on IsItemHovered(): it depends on the last item call site.)
        const bool focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
        const bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_ChildWindows);
        if (!focused || !hovered)
            return;

        // Build camera basis
        glm::vec3 forward = camPtr->m_LookAt - camPtr->m_LookFrom;
        if (glm::length2(forward) < 1e-12f) forward = glm::vec3(0.f, 0.f, -1.f);
        forward = glm::normalize(forward);

        glm::vec3 right = glm::cross(forward, camPtr->m_Up);
        if (glm::length2(right) < 1e-12f) right = glm::vec3(1.f, 0.f, 0.f);
        else right = glm::normalize(right);

        glm::vec3 up = camPtr->m_Up;
        if (glm::length2(up) < 1e-12f) up = glm::vec3(0.f, 1.f, 0.f);

        float distance = glm::length(camPtr->m_LookAt - camPtr->m_LookFrom);

        // ------------------------------- ROTATE (RMB) -------------------------------
        if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
            const glm::vec3 WORLD_UP(0.0f, 1.0f, 0.0f);

            float yaw   = -delta.x * ROTATE_SPEED;
            float pitch = -delta.y * ROTATE_SPEED;

            // Yaw around world up (no roll)
            glm::mat4 yawRot = glm::rotate(glm::mat4(1.0f), yaw, WORLD_UP);
            glm::vec3 fwdAfterYaw = glm::normalize(glm::vec3(yawRot * glm::vec4(forward, 0.0f)));

            // Stable right; reuse last valid right if near poles
            static glm::vec3 s_lastRight = glm::vec3(1, 0, 0);
            glm::vec3 rightAfterYaw = glm::cross(fwdAfterYaw, WORLD_UP);
            float rl2 = glm::dot(rightAfterYaw, rightAfterYaw);
            if (rl2 < 1e-10f) rightAfterYaw = s_lastRight;
            else { rightAfterYaw *= glm::inversesqrt(rl2); s_lastRight = rightAfterYaw; }

            // Clamp pitch to avoid colinearity with WORLD_UP
            float elevation = std::asin(glm::clamp(glm::dot(fwdAfterYaw, WORLD_UP), -1.0f, 1.0f));
            const float epsDeg = 0.8f;
            const float maxEl = glm::radians(90.0f - epsDeg);
            float newElevation = glm::clamp(elevation + pitch, -maxEl, +maxEl);
            float clampedPitch = newElevation - elevation;

            glm::mat4 pitchRot = glm::rotate(glm::mat4(1.0f), clampedPitch, rightAfterYaw);
            glm::vec3 newForward = glm::normalize(glm::vec3(pitchRot * glm::vec4(fwdAfterYaw, 0.0f)));

            glm::vec3 newRight = glm::normalize(glm::cross(newForward, WORLD_UP));
            glm::vec3 newUp    = glm::normalize(glm::cross(newRight, newForward));

            camPtr->m_LookAt = camPtr->m_LookFrom + newForward * distance;
            camPtr->m_Up     = newUp;

            // Refresh basis for subsequent ops this frame
            forward = newForward; right = newRight; up = newUp;
        }

        // -------------------------------- PAN (MMB) --------------------------------
        // Use dt so panning is framerate-independent.
        if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
            glm::vec3 pan = (-delta.x * right + delta.y * up) * (PAN_SPEED * distance * dt);
            camPtr->m_LookFrom += pan;
            camPtr->m_LookAt   += pan;
        }

        // ------------------------------- ZOOM (wheel) ------------------------------
        // Wheel is event-based; usually not scaled by dt (keeps consistent feel).
        if (std::abs(io.MouseWheel) > 0.0f) {
            const float zoomSens = 0.12f;
            const float minDist = 0.05f;
            const float maxDist = 500.0f;

            glm::vec3 offset = camPtr->m_LookFrom - camPtr->m_LookAt;
            float     dist   = glm::max(1e-6f, glm::length(offset));
            glm::vec3 dir    = offset / dist;

            float factor  = std::exp(-io.MouseWheel * zoomSens);
            float newDist = glm::clamp(dist * factor, minDist, maxDist);

            camPtr->m_LookFrom = camPtr->m_LookAt + dir * newDist;

            // Refresh cached values for keyboard move
            distance = newDist;
            forward  = glm::normalize(camPtr->m_LookAt - camPtr->m_LookFrom);
            right    = glm::normalize(glm::cross(forward, camPtr->m_Up));
            up       = glm::normalize(glm::cross(right, forward));
        }

        // ----------------------------- MOVE (Z/Q/S/D) ------------------------------
        // Uses dt for framerate-independent motion. Ignore WantCaptureKeyboard here,
        // since the viewport is focused+hovered (prevents "dead" controls).
        {
            float speed = MOVE_SPEED;
            if (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift)) speed *= 3.0f;
            if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl)  || ImGui::IsKeyDown(ImGuiKey_RightCtrl))  speed *= 0.25f;

            glm::vec3 move(0.0f);
            if (ImGui::IsKeyDown(ImGuiKey_Z)) move += forward; // forward
            if (ImGui::IsKeyDown(ImGuiKey_S)) move -= forward; // backward
            if (ImGui::IsKeyDown(ImGuiKey_Q)) move -= right;   // left
            if (ImGui::IsKeyDown(ImGuiKey_D)) move += right;   // right

            if (glm::length2(move) > 0.0f) {
                move = glm::normalize(move) * (speed * dt);
                camPtr->m_LookFrom += move;
                camPtr->m_LookAt   += move;
            }
        }
    }

    void ClearSelection(Nova::Scene& scene) {
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsItemHovered() && ImGui::IsKeyPressed(ImGuiKey_Escape))
            scene.clearSelection();
    }
    
    void DeleteSelection(Nova::Scene& scene) {
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsItemHovered() && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
            auto selected = scene.getSelected();
            if (!selected.empty()) {
                for (auto entity : selected)
                    scene.destroyEntity(entity);
                scene.clearSelection();
            }
        }
    }

    void Picking(Nova::Scene& scene, ImVec2 size) {
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            // mouse position relative to image quad
            ImVec2 mouse = ImGui::GetMousePos();
            ImVec2 rmin = ImGui::GetItemRectMin();
            ImVec2 rel = ImVec2(mouse.x - rmin.x, mouse.y - rmin.y);

            // camera
            auto camE = scene.getViewportCamera();
            if (camE != entt::null) {
                const auto& cam = scene.registry().get<Nova::Components::CameraComponent>(camE);

                glm::vec2 mouseViewport(rel.x, rel.y);
                glm::vec2 vpSize(size.x, size.y);

                Nova::Math::Ray ray = Nova::Math::mouseClickRayCast(
                    mouseViewport, vpSize,
                    cam.getViewMatrix(),
                    cam.getProjectionMatrix(),
                    cam.m_LookFrom
                );

                ImGuiIO& io = ImGui::GetIO();

                if (auto hit = scene.pickEntity(ray, scene.registry())) {

                    if (io.KeyShift || io.KeyCtrl) {
                        // Ajout à la sélection existante si Maj/Ctrl est enfoncé
                        if (scene.isSelected(hit->entity)) {
                            // Si déjà sélectionné, on le retire
                            scene.removeFromSelection(hit->entity);
                        }
                        else {
                            // Sinon on l'ajoute
                            scene.addToSelection(hit->entity);
                        }
                    }
                    else {
                        // Pas de Maj/Ctrl - sélection simple (remplace la sélection actuelle)
                        scene.clearSelection();
                        scene.addToSelection(hit->entity);
                    }
                }
                else {
                    if (!(io.KeyShift || io.KeyCtrl)) {
                        scene.clearSelection();
                    }
                }
            }
        }
    }

} // namespace Nova::GUI