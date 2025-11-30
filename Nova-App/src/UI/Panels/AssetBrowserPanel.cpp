#include "UI/Panels/AssetBrowserPanel.h"

#include "imgui.h"

namespace Nova::App::UI::Panels::AssetBrowserPanel {

    void Render() {
        ImGui::Begin("Asset Browser");

        ImGui::TextDisabled("TODO: files / assets.");
        ImGui::Separator();
        ImGui::TextUnformatted("Examples :");
        ImGui::BulletText("plane.mesh");
        ImGui::BulletText("checkerboard.texture");
        ImGui::BulletText("example_scene.nova");

        ImGui::End();
    }

} // namespace Nova::App::UI::Panels::AssetBrowserPanel
