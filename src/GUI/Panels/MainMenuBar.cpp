#include "GUI/Panels/MainMenuBar.hpp"
#include "imgui.h"

namespace Nova::GUI {

    void renderMainMenuBar(Nova::Scene& scene) {
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0,0,0,0));
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Project...", "Ctrl+Shift+N")) { /* ... */ }
                if (ImGui::MenuItem("Open Project...", "Ctrl+Shift+O")) { /* ... */ }

                ImGui::Separator();

                if (ImGui::MenuItem("New Scene", "Ctrl+N")) { /* ... */ }
                if (ImGui::MenuItem("Open Scene...", "Ctrl+O")) { /* ... */ }
                if (ImGui::MenuItem("Save Scene", "Ctrl+S")) { /* ... */ }
                if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S")) { /* ... */ }

                ImGui::Separator();

                if (ImGui::MenuItem("Import Asset...")) { /* ... */ }
                if (ImGui::BeginMenu("Export")) {
                    if (ImGui::MenuItem("Selected as GLTF...")) { /* ... */ }
                    if (ImGui::MenuItem("Package...")) { /* ... */ }
                    ImGui::EndMenu();
                }
                ImGui::Separator();

                if (ImGui::MenuItem("Project Settings...")) { /* ... */ }
                if (ImGui::MenuItem("Preferences...")) { /* ... */ }

                ImGui::Separator();

                if (ImGui::MenuItem("Exit", "Alt+F4")) { /* ... */ }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Undo", "Ctrl+Z")) { /* ... */ }
                if (ImGui::MenuItem("Redo", "Ctrl+Y")) { /* ... */ }

                ImGui::Separator();

                if (ImGui::MenuItem("Cut", "Ctrl+X")) { /* ... */ }
                if (ImGui::MenuItem("Copy", "Ctrl+C")) { /* ... */ }
                if (ImGui::MenuItem("Paste", "Ctrl+V")) { /* ... */ }
                if (ImGui::MenuItem("Duplicate", "Ctrl+D")) { /* ... */ }
                if (ImGui::MenuItem("Delete", "Del")) { /* ... */ }

                ImGui::Separator();

                if (ImGui::BeginMenu("Snapping")) {
                    static bool snapT = true; ImGui::MenuItem("Translate", nullptr, &snapT);
                    // ...
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Create")) {
                if (ImGui::MenuItem("Empty Entity")) { /* ecs create */ }
                if (ImGui::BeginMenu("3D Object")) {
                    if (ImGui::MenuItem("Plane")) { /* add MeshComponent.initPlane() */ }
                    if (ImGui::MenuItem("Sphere")) { /* initSphere(...) */ }
                    ImGui::EndMenu();
                }
                if (ImGui::MenuItem("Camera")) { /* createViewportCamera or regular */ }
                if (ImGui::BeginMenu("Light")) {
                    if (ImGui::MenuItem("Directional")) { /* ... */ }
                    if (ImGui::MenuItem("Point")) { /* ... */ }
                    if (ImGui::MenuItem("Spot")) { /* ... */ }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Window")) {
                //to configure interface, reset interface, etc.
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Build")) {
                if (ImGui::MenuItem("Build Project")) { /* ... */ }
                if (ImGui::MenuItem("Build and Run")) { /* ... */ }
                if (ImGui::MenuItem("Build Settings")) { /* ... */ }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Tools")) {
                if (ImGui::MenuItem("Profiler")) { /* ... */ }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("Documentation")) { /* ... */ }
                if (ImGui::MenuItem("About Nova")) { /* ... */ }
                if (ImGui::MenuItem("Check for Updates")) { /* ... */ }
                if (ImGui::MenuItem("Report Issues")) { /* ... */ }
                ImGui::EndMenu();
            }

            // View, Window, Assets, Build, Play, Tools, Help ...
            ImGui::EndMainMenuBar();
        }
        ImGui::PopStyleColor();
    }

} // namespace Nova::GUI