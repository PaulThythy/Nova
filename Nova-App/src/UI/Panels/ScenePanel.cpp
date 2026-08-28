#include "UI/Panels/ScenePanel.h"

#include <algorithm>
#include <cstdio>

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

        const ImVec2 iconSize(18.0f, 18.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 4.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.10f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.20f));

        // Left side: Play/Stop
        if (!playing) {
            void* playIcon = app ? app->GetPlayIconImGuiID() : nullptr;
            const bool clicked = playIcon
                ? ImGui::ImageButton("##Play", playIcon, iconSize)
                : ImGui::Button("Play");
            if (clicked) {
                if (Nova::App::g_AppLayer)
                    Nova::App::g_AppLayer->RequestPlay();
            }
        }
        else {
            void* pauseIcon = app ? app->GetPauseIconImGuiID() : nullptr;
            const bool clicked = pauseIcon
                ? ImGui::ImageButton("##Pause", pauseIcon, iconSize)
                : ImGui::Button("Pause");
            if (clicked) {
                if (Nova::App::g_AppLayer)
                    Nova::App::g_AppLayer->RequestStop();
            }
        }

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        ImGui::EndChild();
    }

    static void DrawViewportSettingsOverlay() {
        AppLayer* app = Nova::App::g_AppLayer;
        if (!app)
            return;

        // Overlay controls in the top-left corner of the rendered viewport.
        ImGui::SetCursorPos(ImVec2(8.0f, 8.0f));

        static const char* kModeNames[] = {
            "Lit",
            "Normals",
            "Positions",
            "Vertex Color",
            "Depth",
        };

        int mode = static_cast<int>(app->GetRenderDebugMode());
        ImGui::SetNextItemWidth(150.0f);
        if (ImGui::Combo("##RenderMode", &mode, kModeNames, static_cast<int>(RenderDebugMode::Count))) {
            auto debugMode = static_cast<RenderDebugMode>(mode);
            app->SetRenderDebugMode(debugMode);
            if (debugMode == RenderDebugMode::Depth)
                app->ShowGrid(false);
        }

        ImGui::SameLine();

        bool showAABB = app->IsAABBVisible();
        if (ImGui::Checkbox("Show AABB", &showAABB))
            app->ShowAABB(showAABB);

        ImGui::SameLine();

        const bool depthMode = (app->GetRenderDebugMode() == RenderDebugMode::Depth);
        bool showGrid = app->IsGridVisible();
        ImGui::BeginDisabled(depthMode);
        if (ImGui::Checkbox("Show Grid", &showGrid))
            app->ShowGrid(showGrid);
        ImGui::EndDisabled();

        // Frame stats in the top-right corner of the viewport.
        const float dt = app->GetDeltaTime();
        const float fps = (dt > 0.0f) ? (1.0f / dt) : 0.0f;
        const float frameMs = dt * 1000.0f;

        char fpsText[32];
        char msText[32];
        std::snprintf(fpsText, sizeof(fpsText), "%.1f FPS", fps);
        std::snprintf(msText, sizeof(msText), "%.2f ms", frameMs);

        const float pad = 8.0f;
        const float textW = (std::max)(ImGui::CalcTextSize(fpsText).x, ImGui::CalcTextSize(msText).x);
        const ImVec2 contentMax = ImGui::GetWindowContentRegionMax();

        ImGui::SetCursorPos(ImVec2(contentMax.x - textW - pad, pad));
        ImGui::TextUnformatted(fpsText);
        ImGui::SetCursorPosX(contentMax.x - ImGui::CalcTextSize(msText).x - pad);
        ImGui::TextUnformatted(msText);

        if (app->HasFocus()) {
            if (const auto focusInfo = app->GetFocusInfo()) {
                char focusName[128];
                std::snprintf(focusName, sizeof(focusName), "Focus: %s", focusInfo->m_Name.c_str());

                char centerText[128];
                std::snprintf(centerText, sizeof(centerText), "Center: %.2f, %.2f, %.2f",
                    focusInfo->m_AabbCenter.x,
                    focusInfo->m_AabbCenter.y,
                    focusInfo->m_AabbCenter.z);

                char sizeText[128];
                std::snprintf(sizeText, sizeof(sizeText), "Size: %.2f, %.2f, %.2f",
                    focusInfo->m_AabbExtents.x * 2.0f,
                    focusInfo->m_AabbExtents.y * 2.0f,
                    focusInfo->m_AabbExtents.z * 2.0f);

                const float focusW = (std::max)({
                    ImGui::CalcTextSize(focusName).x,
                    ImGui::CalcTextSize(centerText).x,
                    ImGui::CalcTextSize(sizeText).x,
                });

                char escapeText[128];
                std::snprintf(escapeText, sizeof(escapeText), "Press ESC to quit focus mode");

                const float lineH = ImGui::GetTextLineHeightWithSpacing();
                float y = pad + lineH * 2.0f;

                ImGui::SetCursorPos(ImVec2(contentMax.x - focusW - pad, y));
                ImGui::TextUnformatted(focusName);
                y += lineH;

                ImGui::SetCursorPos(ImVec2(contentMax.x - ImGui::CalcTextSize(centerText).x - pad, y));
                ImGui::TextUnformatted(centerText);
                y += lineH;

                ImGui::SetCursorPos(ImVec2(contentMax.x - ImGui::CalcTextSize(sizeText).x - pad, y));
                ImGui::TextUnformatted(sizeText);
                y += lineH;

                ImGui::SetCursorPos(ImVec2(contentMax.x - ImGui::CalcTextSize(escapeText).x - pad, y));
                ImGui::TextUnformatted(escapeText);
            }
        }
    }

    void Render(const std::string& sceneName) {
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::Begin(sceneName.c_str(), nullptr, flags);

        // 1) Toolbar INSIDE the scene panel (requested)
        DrawSceneToolbarBar();

        // 2) Viewport (framebuffer) with settings overlaid on top
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
            if (void* textureId = Nova::App::g_AppLayer->GetRenderer()->GetTextureImGuiID(Nova::App::g_AppLayer->GetSceneColor())) {
                ImGui::Image(textureId, size, ImVec2(0, 0), ImVec2(1, 1));

                // Left-click: select. Double-click: focus (orbit camera on object AABB).
                if (ImGui::IsItemHovered()) {
                    const ImVec2 mouse = ImGui::GetMousePos();
                    const ImVec2 min = ImGui::GetItemRectMin();
                    const ImVec2 max = ImGui::GetItemRectMax();
                    const float w = max.x - min.x;
                    const float h = max.y - min.y;
                    if (w > 1e-3f && h > 1e-3f) {
                        const float u = (mouse.x - min.x) / w;
                        const float v = (mouse.y - min.y) / h;

                        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                            Nova::App::g_AppLayer->FocusAtViewportUV(u, v);
                        } else if (!Nova::App::g_AppLayer->HasFocus() && ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                            const bool addToSelection = ImGui::IsKeyDown(ImGuiKey_LeftShift);
                            Nova::App::g_AppLayer->PickAtViewportUV(u, v, addToSelection);
                        }
                    }
                }
            }
        }
        else {
            ImGui::TextUnformatted("Framebuffer not ready.");
        }

        DrawViewportSettingsOverlay();

        ImGui::EndChild();
        ImGui::PopStyleVar();

        ImGui::End();
    }

} // namespace Nova::App::UI::Panels::ScenePanel