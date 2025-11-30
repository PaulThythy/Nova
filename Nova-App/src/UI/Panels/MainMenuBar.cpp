#include "UI/Panels/MainMenuBar.h"

#include "imgui.h"

namespace Nova::App::UI::Panels::MainMenuBar {

    void Render() {
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                ImGui::MenuItem("New Project", "Ctrl+Shift+N");
                ImGui::MenuItem("Open Project", "Ctrl+Shift+O");

                ImGui::Separator();

                ImGui::MenuItem("New Scene", "Ctrl+N");
                ImGui::MenuItem("Open Scene", "Ctrl+O");
                ImGui::MenuItem("Save Scene", "Ctrl+S");
                ImGui::MenuItem("Save Scene As", "Ctrl+Shift+S");

                ImGui::Separator();

                ImGui::MenuItem("Import Asset");
                if (ImGui::BeginMenu("Export")) {
                    ImGui::MenuItem("Selected as GLTF");
                    ImGui::MenuItem("Package");
                    ImGui::EndMenu();
                }

                ImGui::Separator();
                ImGui::MenuItem("Project Settings");
                ImGui::MenuItem("Preferences");

                ImGui::Separator();
                ImGui::MenuItem("Exit", "Alt+F4");

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit")) {
                ImGui::MenuItem("Undo", "Ctrl+Z");
                ImGui::MenuItem("Redo", "Ctrl+Y");

                ImGui::Separator();
                ImGui::MenuItem("Cut", "Ctrl+X");
                ImGui::MenuItem("Copy", "Ctrl+C");
                ImGui::MenuItem("Paste", "Ctrl+V");
                ImGui::MenuItem("Duplicate", "Ctrl+D");
                ImGui::MenuItem("Delete", "Del");

                ImGui::Separator();
                if (ImGui::BeginMenu("Snapping")) {
                    static bool snapT = true;
                    ImGui::MenuItem("Translate", nullptr, &snapT);
                    ImGui::EndMenu();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Create")) {
                ImGui::MenuItem("Empty Entity");
                if (ImGui::BeginMenu("3D Object")) {
                    ImGui::MenuItem("Plane");
                    ImGui::MenuItem("Sphere");
                    ImGui::EndMenu();
                }
                ImGui::MenuItem("Camera");
                if (ImGui::BeginMenu("Light")) {
                    ImGui::MenuItem("Directional");
                    ImGui::MenuItem("Point");
                    ImGui::MenuItem("Spot");
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Window")) {
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Build")) {
                ImGui::MenuItem("Build Project");
                ImGui::MenuItem("Build and Run");
                ImGui::MenuItem("Build Settings");
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Tools")) {
                ImGui::MenuItem("Profiler");
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help")) {
                ImGui::MenuItem("Documentation");
                ImGui::MenuItem("About Nova");
                ImGui::MenuItem("Check for Updates");
                ImGui::MenuItem("Report Issues");
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        ImGui::PopStyleColor();
    }

} // namespace Nova::App::UI::Panels::MainMenuBar
