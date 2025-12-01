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

        static ImVec2 s_LastSize = ImVec2(-1.0f, -1.0f);
        if (size.x > 0.0f && size.y > 0.0f &&
            (size.x != s_LastSize.x || size.y != s_LastSize.y)) {

            s_LastSize = size;

            using namespace Nova::Core::Events;
            ImGuiPanelResizeEvent e("Viewport", size.x, size.y);
            Nova::Core::Application::Get().OnEvent(e);
        }

        if (Nova::App::g_AppLayer) {
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
