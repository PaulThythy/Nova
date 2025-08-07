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

        if (camPtr && viewportFocused)
        {
            glm::vec3 forward = glm::normalize(camPtr->m_LookAt - camPtr->m_LookFrom);
            glm::vec3 right   = glm::normalize(glm::cross(forward, camPtr->m_Up));
            glm::vec3 up      = camPtr->m_Up;

            float distance = glm::length(camPtr->m_LookAt - camPtr->m_LookFrom);

            // ------------------------------------------------------------- ROTATE
            if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
            {
                float yaw   = -delta.x * ROTATE_SPEED;
                float pitch = -delta.y * ROTATE_SPEED;

                glm::mat4 rot = glm::rotate(glm::mat4(1.f), yaw,   up)   *
                                glm::rotate(glm::mat4(1.f), pitch, right);

                glm::vec3 offset = camPtr->m_LookFrom - camPtr->m_LookAt;
                offset           = glm::vec3(rot * glm::vec4(offset, 1.f));

                camPtr->m_LookFrom = camPtr->m_LookAt + offset;
            }

            // --------------------------------------------------------------- PAN
            if (io.KeyShift && ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                glm::vec3 pan =
                    (-delta.x * right + delta.y * up) * PAN_SPEED * distance;
                camPtr->m_LookFrom += pan;
                camPtr->m_LookAt   += pan;
            }

            // -------------------------------------------------------------- ZOOM
            if (std::abs(io.MouseWheel) > 0.0f)
            {
                float zoom = -io.MouseWheel * ZOOM_SPEED;
                camPtr->m_LookFrom += forward * zoom;
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