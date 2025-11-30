#include "UI/Panels/ViewportPanel.h"
#include "App/AppLayer.h"

#include "imgui.h"

namespace Nova::App::UI::Panels::ViewportPanel {

    void Render() {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Viewport",
            nullptr,
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse);

        ImVec2 size = ImGui::GetContentRegionAvail();

        if (Nova::App::g_AppLayer) {
            Nova::App::g_AppLayer->SetViewportSize(size.x, size.y);

            GLuint texID = Nova::App::g_AppLayer->GetViewportTexture();

            if (texID != 0) {
                ImGui::Image(
                    (ImTextureID)(uintptr_t)texID,
                    size,
                    ImVec2(0.0f, 1.0f),
                    ImVec2(1.0f, 0.0f)
                );
            }
            else {
                ImGui::TextUnformatted("Framebuffer not ready.");
            }
        }
        else {
            ImGui::TextUnformatted("AppLayer not available.");
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

} // namespace Nova::App::UI::Panels::ViewportPanel
