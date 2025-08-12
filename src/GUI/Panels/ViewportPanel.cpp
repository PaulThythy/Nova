#include "GUI/Panels/ViewportPanel.hpp"
#include "Renderer/Renderer.hpp"
#include "imgui.h"
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

        constexpr float ROTATE_SPEED = 0.005f;
        constexpr float PAN_SPEED    = 0.002f;
        constexpr float ZOOM_SPEED   = 1.0f;

        static ImVec2 lastMousePos {0,0};

        auto camE = scene.getViewportCamera();
        Nova::Components::CameraComponent* camPtr = nullptr;
        if (camE != entt::null)
            camPtr = &scene.registry().get<Nova::Components::CameraComponent>(camE);

        const bool viewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsItemHovered();
        ImGuiIO& io = ImGui::GetIO();
        ImVec2 mousePos = io.MousePos;
        ImVec2 delta    = { mousePos.x - lastMousePos.x,
                            mousePos.y - lastMousePos.y };
        lastMousePos    = mousePos;

        if (viewportFocused && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
            auto selected = scene.getSelected();
            if (!selected.empty()) {
                for (auto entity : selected)
                    scene.destroyEntity(entity);
                scene.clearSelection();
            }
        }

        if (camPtr && viewportFocused)
        {
            glm::vec3 forward = glm::normalize(camPtr->m_LookAt - camPtr->m_LookFrom);
            glm::vec3 right   = glm::normalize(glm::cross(forward, camPtr->m_Up));
            glm::vec3 up      = camPtr->m_Up;

            float distance = glm::length(camPtr->m_LookAt - camPtr->m_LookFrom);

            // ------------------------------------------------------------- ROTATE
            if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
            {
                float yaw = -delta.x * ROTATE_SPEED;
                float pitch = -delta.y * ROTATE_SPEED;

                float dist = glm::length(camPtr->m_LookAt - camPtr->m_LookFrom);

                glm::mat4 yawRot = glm::rotate(glm::mat4(1.0f), yaw, camPtr->m_Up);
                glm::mat4 pitchRot = glm::rotate(glm::mat4(1.0f), pitch, right);

                // Forward -> yaw rotation then pitch
                glm::vec3 fwd = glm::normalize(glm::vec3(yawRot * glm::vec4(forward, 0.0f)));
                fwd = glm::normalize(glm::vec3(pitchRot * glm::vec4(fwd, 0.0f)));

                // up auto update (no roll lock)
                glm::vec3 newRight = glm::normalize(glm::cross(fwd, camPtr->m_Up));
                glm::vec3 newUp = glm::normalize(glm::cross(newRight, fwd));

                // update camera
                camPtr->m_LookAt = camPtr->m_LookFrom + fwd * distance;
                camPtr->m_Up = newUp;
            }

            // --------------------------------------------------------------- PAN
            if (ImGui::IsMouseDown(ImGuiMouseButton_Middle))
            {
                glm::vec3 pan = (-delta.x * right + delta.y * up) * PAN_SPEED * distance;
                camPtr->m_LookFrom += pan;
                camPtr->m_LookAt   += pan;
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

        if (viewportFocused && ImGui::IsKeyPressed(ImGuiKey_Escape))
            scene.clearSelection();

        // ----- Picking -----
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            // mouse position relative to image quad
            ImVec2 mouse = ImGui::GetMousePos();
            ImVec2 rmin  = ImGui::GetItemRectMin();
            ImVec2 rel   = ImVec2(mouse.x - rmin.x, mouse.y - rmin.y);

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

                if (auto hit = scene.pickEntity(ray, scene.registry())) {
                    ImGuiIO& io = ImGui::GetIO();

                    if (io.KeyShift || io.KeyCtrl) {
                        // Ajout à la sélection existante si Maj/Ctrl est enfoncé
                        if (scene.isSelected(hit->entity)) {
                            // Si déjà sélectionné, on le retire
                            scene.removeFromSelection(hit->entity);
                        } else {
                            // Sinon on l'ajoute
                            scene.addToSelection(hit->entity);
                        }
                    } else {
                        // Pas de Maj/Ctrl - sélection simple (remplace la sélection actuelle)
                        scene.clearSelection();
                        scene.addToSelection(hit->entity);
                    }
                } else {
                    if (!(io.KeyShift || io.KeyCtrl)) {
                        scene.clearSelection();
                    }
                }
            }
        }
        // ----- end picking -----

        ImGui::End();
        ImGui::PopStyleVar();
    }

} // namespace Nova::GUI