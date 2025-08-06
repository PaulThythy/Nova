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

        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
        ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            scene.clearSelected();
        }

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
                    auto tag = scene.registry().try_get<Nova::Components::TagComponent>(hit->entity);
                    std::cout << "Hit entity " << (tag ? tag->m_Tag : "<un-tagged>") << '\n';
                    scene.setSelected(hit->entity);
                }
            }
        }
        // ----- end picking -----

        ImGui::End();
        ImGui::PopStyleVar();
    }

} // namespace Nova::GUI