#include "GUI/Panels/HierarchyPanel.hpp"
#include "imgui.h"

namespace Nova::GUI {

    void renderHierarchyPanel() {
        ImGui::Begin("Hierarchy");

        static int selectedIndex = -1;

        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
            ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            selectedIndex = -1;
        }

        const char* items[] = { "Camera", "Light", "Cube", "Sphere" };
        int itemCount = IM_ARRAYSIZE(items);

        ImGui::Text("Scene");
        ImGui::Separator();

        for (int i = 0; i < itemCount; i++) {
            bool isSelected = (selectedIndex == i);
            if (ImGui::Selectable(items[i], isSelected)) {
                selectedIndex = i;
            }
        }

        ImGui::End();
    }

} // namespace Nova::GUI