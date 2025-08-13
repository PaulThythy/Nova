#include "GUI/Panels/ViewportPanel.hpp"
#include "Renderer/Renderer.hpp"
#include "Math/Ray.hpp"
#include "Math/AABB.hpp"
#include "Components/TransformComponent.hpp"
#include "Components/MeshComponent.hpp"

#include <iostream>

namespace Nova::GUI {

    void renderViewportPanel(Nova::Renderer::IRenderer* renderer, Nova::Scene& scene) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImVec2 size = ImGui::GetContentRegionAvail();
        renderer->updateViewportSize((int)size.x, (int)size.y);

        ImGui::Image((ImTextureID)renderer->getImGuiTextureID(), size, ImVec2(0, 1), ImVec2(1, 0));

        controls(scene);

        deleteSelection(scene);

        clearSelection(scene);

        picking(scene, size);

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void controls(Nova::Scene& scene) {
        constexpr float ROTATE_SPEED = 0.005f;
        constexpr float PAN_SPEED = 0.002f;
        constexpr float ZOOM_SPEED = 1.0f;

        static ImVec2 lastMousePos{ 0,0 };

        auto camE = scene.getViewportCamera();
        Nova::Components::CameraComponent* camPtr = nullptr;
        if (camE != entt::null)
            camPtr = &scene.registry().get<Nova::Components::CameraComponent>(camE);

        ImGuiIO& io = ImGui::GetIO();
        ImVec2 mousePos = io.MousePos;
        ImVec2 delta = { mousePos.x - lastMousePos.x,
                            mousePos.y - lastMousePos.y };
        lastMousePos = mousePos;

        if (camPtr && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsItemHovered())
        {
            glm::vec3 forward = glm::normalize(camPtr->m_LookAt - camPtr->m_LookFrom);
            glm::vec3 right = glm::normalize(glm::cross(forward, camPtr->m_Up));
            glm::vec3 up = camPtr->m_Up;

            float distance = glm::length(camPtr->m_LookAt - camPtr->m_LookFrom);

            // ------------------------------------------------------------- ROTATE
            if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
            {
                float yaw = -delta.x * ROTATE_SPEED;
                float pitch = -delta.y * ROTATE_SPEED;

                const glm::vec3 WORLD_UP(0.0f, 1.0f, 0.0f);
                float distance = glm::length(camPtr->m_LookAt - camPtr->m_LookFrom);

                // Current forward
                glm::vec3 forward = glm::normalize(camPtr->m_LookAt - camPtr->m_LookFrom);

                // 1) Yaw around WORLD_UP (no roll)
                glm::mat4 yawRot = glm::rotate(glm::mat4(1.0f), yaw, WORLD_UP);
                glm::vec3 fwdAfterYaw = glm::normalize(glm::vec3(yawRot * glm::vec4(forward, 0.0f)));

                // Stable right; reuse last valid right if near the pole
                static glm::vec3 s_lastRight = glm::vec3(1, 0, 0);
                glm::vec3 right = glm::cross(fwdAfterYaw, WORLD_UP);
                float rl2 = glm::dot(right, right);
                if (rl2 < 1e-10f) {
                    right = s_lastRight; // fallback
                }
                else {
                    right *= glm::inversesqrt(rl2);
                    s_lastRight = right;
                }

                // 2) Clamp pitch so we never hit colinearity with WORLD_UP
                //    elevation = asin(dot(fwd, WORLD_UP)) in [-pi/2, +pi/2]
                float elevation = std::asin(glm::clamp(glm::dot(fwdAfterYaw, WORLD_UP), -1.0f, 1.0f));
                const float epsDeg = 0.8f;                       // "safety margin" from the pole
                const float maxEl = glm::radians(90.0f - epsDeg);
                float newElevation = glm::clamp(elevation + pitch, -maxEl, +maxEl);
                float clampedPitch = newElevation - elevation;

                // 3) Apply clamped pitch around right
                glm::mat4 pitchRot = glm::rotate(glm::mat4(1.0f), clampedPitch, right);
                glm::vec3 newForward = glm::normalize(glm::vec3(pitchRot * glm::vec4(fwdAfterYaw, 0.0f)));

                // Rebuild ortho-normal basis (no roll): up is derived from WORLD_UP
                glm::vec3 newRight = glm::normalize(glm::cross(newForward, WORLD_UP));
                glm::vec3 newUp = glm::normalize(glm::cross(newRight, newForward));

                // Update camera (pivot at LookFrom)
                camPtr->m_LookAt = camPtr->m_LookFrom + newForward * distance;
                camPtr->m_Up = newUp;
            }

            // --------------------------------------------------------------- PAN
            if (ImGui::IsMouseDown(ImGuiMouseButton_Middle))
            {
                glm::vec3 pan = (-delta.x * right + delta.y * up) * PAN_SPEED * distance;
                camPtr->m_LookFrom += pan;
                camPtr->m_LookAt += pan;
            }

            // -------------------------------------------------------------- ZOOM
            if (std::abs(io.MouseWheel) > 0.0f)
            {
                const float zoomSens = 0.12f;               // wheel sensitivity per notch
                const float minDist = 0.05f;                // clamp: don't get stuck on target
                const float maxDist = 500.0f;               // optional far clamp

                glm::vec3 offset = camPtr->m_LookFrom - camPtr->m_LookAt;
                float     dist = glm::max(1e-6f, glm::length(offset));
                glm::vec3 dir = offset / dist;

                // scale distance exponentially (wheel>0 => factor<1 => zoom in)
                float factor = std::exp(-io.MouseWheel * zoomSens);
                float newDist = glm::clamp(dist * factor, minDist, maxDist);

                camPtr->m_LookFrom = camPtr->m_LookAt + dir * newDist;

                // update cached values for next pan the same frame
                distance = newDist;
                forward = glm::normalize(camPtr->m_LookAt - camPtr->m_LookFrom);
                right = glm::normalize(glm::cross(forward, camPtr->m_Up));
                up = glm::normalize(glm::cross(right, forward));
            }
        }
    }

    void clearSelection(Nova::Scene& scene) {
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsItemHovered() && ImGui::IsKeyPressed(ImGuiKey_Escape))
            scene.clearSelection();
    }
    
    void deleteSelection(Nova::Scene& scene) {
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsItemHovered() && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
            auto selected = scene.getSelected();
            if (!selected.empty()) {
                for (auto entity : selected)
                    scene.destroyEntity(entity);
                scene.clearSelection();
            }
        }
    }

    void picking(Nova::Scene& scene, ImVec2 size) {
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