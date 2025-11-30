#include "UI/Panels/HierarchyPanel.h"

#include "imgui.h"

namespace Nova::App::UI::Panels::HierarchyPanel {

    void Render() {
        ImGui::Begin("Hierarchy");

        ImGui::TextDisabled("TODO: display list of entities.");
        ImGui::Separator();
        ImGui::TextUnformatted("placeholder.");

        ImGui::End();
    }

} // namespace Nova::App::UI::Panels::HierarchyPanel
