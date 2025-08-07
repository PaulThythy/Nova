#include "GUI/Panels/HierarchyPanel.hpp"
#include "imgui.h"

namespace Nova::GUI {

    void renderHierarchyPanel(Nova::Scene& scene) {
        ImGui::Begin("Hierarchy");

        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            scene.clearSelected();
        }

        auto& reg = scene.registry();

        for (auto e : reg.storage<entt::entity>()) {
            if (auto cam = reg.try_get<Components::CameraComponent>(e)) {
                if (cam->m_IsViewportCamera) continue;
            }

            const char* name = "Entity";
            if (auto tag = reg.try_get<Components::TagComponent>(e)) {
                if (!tag->m_Tag.empty()) name = tag->m_Tag.c_str();
            }

            const bool isSelected = (scene.getSelected() == e);

            ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_Leaf |
                ImGuiTreeNodeFlags_NoTreePushOnOpen |
                ImGuiTreeNodeFlags_SpanAvailWidth |
                (isSelected ? ImGuiTreeNodeFlags_Selected : 0);

            ImGui::PushID((int)entt::to_integral(e));
            ImGui::TreeNodeEx((void*)(intptr_t)entt::to_integral(e), flags, "%s", name);

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                scene.setSelected(e);
            }

            if (ImGui::BeginPopupContextItem("entity_ctx")) {
                if (ImGui::MenuItem("Select")) {
                    scene.setSelected(e);
                }

                ImGui::EndPopup();
            }

            ImGui::PopID();
        }

        ImGui::End();
    }

} // namespace Nova::GUI