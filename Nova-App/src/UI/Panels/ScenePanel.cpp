#include "UI/Panels/ScenePanel.h"

#include "imgui.h"
#include "App/AppLayer.h"
#include "Events/ApplicationEvents.h"
#include "Core/Application.h"

namespace Nova::App::UI::Panels::ScenePanel {

    static void DrawSceneToolbarBar() {
        // A header-like bar INSIDE the scene window, just under the title.
        const float barH = 34.0f;

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::BeginChild("##SceneToolbarBar", ImVec2(0.0f, barH), true, flags);

        AppLayer* app = Nova::App::g_AppLayer;
        const bool playing = app && (app->GetSceneState() == AppLayer::SceneState::Play);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 6.0f));

        // Left side: Play/Stop
        /*if (!playing) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.70f, 0.20f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.80f, 0.25f, 1.0f));
            if (ImGui::Button("->")) {
                if (Nova::App::g_AppLayer)
                    Nova::App::g_AppLayer->RequestPlay();
            }
            ImGui::PopStyleColor(2);
        }
        else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.20f, 0.20f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.25f, 0.25f, 1.0f));
            if (ImGui::Button("||")) {
                if (Nova::App::g_AppLayer)
                    Nova::App::g_AppLayer->RequestStop();
            }
            ImGui::PopStyleColor(2);
        }*/

        ImGui::PopStyleVar();

        ImGui::EndChild();
    }

    static void DrawViewportSettingsBar() {
        const float barH = 32.0f;

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::BeginChild("##ViewportSettingsBar", ImVec2(0.0f, barH), true, flags);

        static int s_CameraMode = 0; // 0: Perspective, 1: Orthographic
        static int s_RenderMode = 0; // 0: Lit, 1: Unlit, 2: Wireframe

        const char* camItems[] = { "Perspective", "Orthographic" };
        const char* rndItems[] = { "Lit", "Unlit", "Wireframe" };

        ImGui::SetNextItemWidth(150.0f);
        ImGui::Combo("##cam", &s_CameraMode, camItems, IM_ARRAYSIZE(camItems));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(130.0f);
        ImGui::Combo("##rnd", &s_RenderMode, rndItems, IM_ARRAYSIZE(rndItems));

        ImGui::EndChild();
    }

    void Render(const std::string& sceneName) {
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::Begin(sceneName.c_str(), nullptr, flags);

        // 1) Toolbar INSIDE the scene panel (requested)
        DrawSceneToolbarBar();

        // 2) Viewport settings bar
        DrawViewportSettingsBar();

        // 3) Viewport (framebuffer)
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::BeginChild("##Viewport", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        // Expose hover state to AppLayer so orbit-camera input is restricted to this area.
        if (Nova::App::g_AppLayer)
            Nova::App::g_AppLayer->SetViewportHovered(ImGui::IsWindowHovered());

        ImVec2 size = ImGui::GetContentRegionAvail();

        static ImVec2 s_LastSize(-1.0f, -1.0f);
        if (size.x > 0.0f && size.y > 0.0f &&
            (size.x != s_LastSize.x || size.y != s_LastSize.y)) {

            s_LastSize = size;

            // Keep the same resize pipeline name: "Viewport"
            using namespace Nova::Core::Events;
            ImGuiPanelResizeEvent e("Viewport", size.x, size.y);
            Nova::Core::Application::Get().OnEvent(e);
        }

        if (Nova::App::g_AppLayer->GetRenderer()) {
            if (void* textureId = Nova::App::g_AppLayer->GetRenderer()->GetViewportTextureID()) {
                // OpenGL FBOs have Y=0 at the bottom, so a V-flip is required.
                // Vulkan/Metal/DX have Y=0 at the top: no flip needed.
                const GraphicsAPI api = Nova::Core::Application::Get().GetWindow().GetGraphicsAPI();
                const bool needsVFlip = (api == GraphicsAPI::OpenGL);
                const ImVec2 uv0 = needsVFlip ? ImVec2(0, 1) : ImVec2(0, 0);
                const ImVec2 uv1 = needsVFlip ? ImVec2(1, 0) : ImVec2(1, 1);
                ImGui::Image(textureId, size, uv0, uv1);
            }
        }
        else {
            ImGui::TextUnformatted("Framebuffer not ready.");
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();

        ImGui::End();
    }

} // namespace Nova::App::UI::Panels::ScenePanel
