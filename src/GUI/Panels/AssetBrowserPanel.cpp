#include "GUI/Panels/InspectorPanel.hpp"
#include "imgui.h"

namespace Nova::GUI {

    void renderAssetBrowserPanel() {
        ImGui::Begin("Asset Browser");
        ImGui::Text("asset_1.obj");
        ImGui::Text("material_wood.mat");
        ImGui::Text("scene_01.nova");
        ImGui::End();
    }

} // namespace Nova::GUI