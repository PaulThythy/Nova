#include "UI/Panels/InspectorPanel.h"

#include "imgui.h"

namespace Nova::App::UI::Panels::InspectorPanel {

    void Render() {
        ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoScrollbar);

        ImGui::TextDisabled("TODO: display components of selected entities.");
        ImGui::Separator();
        ImGui::TextUnformatted("No ECS integration yet.");

        ImGui::End();
    }

} // namespace Nova::App::UI::Panels::InspectorPanel
