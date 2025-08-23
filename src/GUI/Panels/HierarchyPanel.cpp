#include "GUI/Panels/HierarchyPanel.hpp"
#include "imgui.h"

namespace Nova::GUI::HierarchyPanel {

    void Render(Nova::Scene& scene) {
        ImGui::Begin("Hierarchy");

        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            scene.clearSelection();
        }

        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {

            if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
                auto selected = scene.getSelected();
                if (!selected.empty()) {
                    for (auto entity : selected) {
                        scene.destroyEntity(entity);
                    }
                    scene.clearSelection();
                }
            }

            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                scene.clearSelection();
            }
        }

        auto& reg = scene.registry();

        for (auto e : reg.storage<entt::entity>()) {
            if (!reg.valid(e))
                continue;

            if (auto cam = reg.try_get<Components::CameraComponent>(e)) {
                if (cam->m_IsViewportCamera) continue;
            }

            const char* name = "Entity";
            if (auto tag = reg.try_get<Components::TagComponent>(e)) {
                if (!tag->m_Tag.empty()) name = tag->m_Tag.c_str();
            }

            const bool isSelected = scene.isSelected(e);

            ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_Leaf |
                ImGuiTreeNodeFlags_NoTreePushOnOpen |
                ImGuiTreeNodeFlags_SpanAvailWidth |
                (isSelected ? ImGuiTreeNodeFlags_Selected : 0);

            ImGui::PushID((int)entt::to_integral(e));
            ImGui::TreeNodeEx((void*)(intptr_t)entt::to_integral(e), flags, "%s", name);

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                bool ctrlPressed = ImGui::GetIO().KeyCtrl;
                bool shiftPressed = ImGui::GetIO().KeyShift;

                if (ctrlPressed || shiftPressed) {
                    // multiple selection 
                    if(scene.isSelected(e)) {
                        scene.removeFromSelection(e);
                    } else {
                        scene.addToSelection(e);
                    }
                } else {
                    // simple selection
                    scene.clearSelection();
                    scene.addToSelection(e);
                }
            }

            ImGui::PopID();
        }

        ImGui::End();
    }

} // namespace Nova::GUI