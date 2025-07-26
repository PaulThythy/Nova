#include "GUI/EditorUI.hpp"

namespace Nova {
    namespace GUI {

        void setupDockSpace(bool useIni, ImGuiID dockspace_id) {
            static bool dock_initialized = false;
            if (!dock_initialized && !useIni && ImGui::DockBuilderGetNode(dockspace_id)) {
                dock_initialized = true;
                ImGui::DockBuilderRemoveNode(dockspace_id);
                ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

                ImGuiID dock_down = 0, dock_main = 0;
                ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.19f, &dock_down, &dock_main);

                ImGuiID dock_right = 0, dock_center = 0; 
                ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.25f, &dock_right, &dock_center);

                ImGuiID dock_right_top = 0, dock_right_bottom = 0;
                ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Up, 0.5f, &dock_right_top, &dock_right_bottom);

                ImGui::DockBuilderDockWindow("Hierarchy", dock_right_top);
                ImGui::DockBuilderDockWindow("Inspector", dock_right_bottom);
                ImGui::DockBuilderDockWindow("Viewport",  dock_center);
                ImGui::DockBuilderDockWindow("Asset Browser", dock_down);

                ImGui::DockBuilderFinish(dockspace_id);
            }
        }

        void renderHierarchyPanel() {
            ImGui::Begin("Hierarchy");
            ImGui::Text("Scene");
            ImGui::Separator();
            ImGui::Selectable("Camera");
            ImGui::Selectable("Light");
            ImGui::Selectable("Cube");
            ImGui::Selectable("Sphere");
            ImGui::End();
        }

        void renderViewportPanel(GLuint viewportTexture) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            ImVec2 size = ImGui::GetContentRegionAvail();
            ImGui::Image((ImTextureID)viewportTexture, size);
            ImGui::End();
            ImGui::PopStyleVar();
        }
        
        void renderInspectorPanel() {
            ImGui::Begin("Inspector");
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Separator();

                ImGuiStyle& style = ImGui::GetStyle();
                float availW      = ImGui::GetContentRegionAvail().x;
                float badgeW      = 2.0f;
                float spacing     = style.ItemInnerSpacing.x;
                float groupSpacing = style.ItemSpacing.x;
                float rightMargin = style.WindowPadding.x;
                float usableW = availW - rightMargin;

                float totalGapsX    = spacing * 3 + groupSpacing * 2;   
                float totalBadgesW  = badgeW * 3;
                float inputW        = (usableW - totalBadgesW - totalGapsX) / 3.0f;

                // --- POSITION ---
                {
                    static float position[3] = { 0.0f, 0.0f, 0.0f };
                    ImGui::Text("Position");
                    ImGui::Spacing();

                    // X
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1,0,0,1));
                    ImGui::Button("X##posBadge", ImVec2(badgeW,0));
                    ImGui::PopStyleColor();
                    ImGui::SameLine();
                    ImGui::PushItemWidth(inputW);
                    ImGui::InputFloat("##posX", &position[0], 0.0f, 0.0f, "%.6f");
                    ImGui::PopItemWidth();
                    ImGui::SameLine(0, groupSpacing);

                    // Y
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,1,0,1));
                    ImGui::Button("Y##posBadge", ImVec2(badgeW,0));
                    ImGui::PopStyleColor();
                    ImGui::SameLine();
                    ImGui::PushItemWidth(inputW);
                    ImGui::InputFloat("##posY", &position[1], 0.0f, 0.0f, "%.6f");
                    ImGui::PopItemWidth();
                    ImGui::SameLine(0, groupSpacing);

                    // Z
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,1,1));
                    ImGui::Button("Z##posBadge", ImVec2(badgeW,0));
                    ImGui::PopStyleColor();
                    ImGui::SameLine();
                    ImGui::PushItemWidth(inputW);
                    ImGui::InputFloat("##posZ", &position[2], 0.0f, 0.0f, "%.6f");
                    ImGui::PopItemWidth();
                }

                ImGui::Separator();

                // --- ROTATION ---
                {
                    static float rotation[3] = { 0.0f, 0.0f, 0.0f };
                    ImGui::Text("Rotation");
                    ImGui::Spacing();

                    // X
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1,0,0,1));
                    ImGui::Button("X##rotBadge", ImVec2(badgeW,0));
                    ImGui::PopStyleColor();
                    ImGui::SameLine();
                    ImGui::PushItemWidth(inputW);
                    ImGui::InputFloat("##rotX", &rotation[0], 0.0f, 0.0f, "%.6f");
                    ImGui::PopItemWidth();
                    ImGui::SameLine(0, groupSpacing);

                    // Y
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,1,0,1));
                    ImGui::Button("Y##rotBadge", ImVec2(badgeW,0));
                    ImGui::PopStyleColor();
                    ImGui::SameLine();
                    ImGui::PushItemWidth(inputW);
                    ImGui::InputFloat("##rotY", &rotation[1], 0.0f, 0.0f, "%.6f");
                    ImGui::PopItemWidth();
                    ImGui::SameLine(0, groupSpacing);

                    // Z
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,1,1));
                    ImGui::Button("Z##rotBadge", ImVec2(badgeW,0));
                    ImGui::PopStyleColor();
                    ImGui::SameLine();
                    ImGui::PushItemWidth(inputW);
                    ImGui::InputFloat("##rotZ", &rotation[2], 0.0f, 0.0f, "%.6f");
                    ImGui::PopItemWidth();
                }

                ImGui::Separator();

                // --- SCALE ---
                {
                    static float scale[3] = { 1.0f, 1.0f, 1.0f };
                    ImGui::Text("Scale");
                    ImGui::Spacing();

                    // X
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1,0,0,1));
                    ImGui::Button("X##sclBadge", ImVec2(badgeW,0));
                    ImGui::PopStyleColor();
                    ImGui::SameLine();
                    ImGui::PushItemWidth(inputW);
                    ImGui::InputFloat("##sclX", &scale[0], 0.0f, 0.0f, "%.6f");
                    ImGui::PopItemWidth();
                    ImGui::SameLine(0, groupSpacing);

                    // Y
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,1,0,1));
                    ImGui::Button("Y##sclBadge", ImVec2(badgeW,0));
                    ImGui::PopStyleColor();
                    ImGui::SameLine();
                    ImGui::PushItemWidth(inputW);
                    ImGui::InputFloat("##sclY", &scale[1], 0.0f, 0.0f, "%.6f");
                    ImGui::PopItemWidth();
                    ImGui::SameLine(0, groupSpacing);

                    // Z
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,1,1));
                    ImGui::Button("Z##sclBadge", ImVec2(badgeW,0));
                    ImGui::PopStyleColor();
                    ImGui::SameLine();
                    ImGui::PushItemWidth(inputW);
                    ImGui::InputFloat("##sclZ", &scale[2], 0.0f, 0.0f, "%.6f");
                    ImGui::PopItemWidth();
                }
            }
            ImGui::End();
        }

        void renderAssetBrowserPanel() {
            ImGui::Begin("Asset Browser");
            ImGui::Text("asset_1.obj");
            ImGui::Text("material_wood.mat");
            ImGui::Text("scene_01.nova");
            ImGui::End();
        }

        void render(GLuint viewportTexture) {
            bool useIni = (bool)(intptr_t)ImGui::GetIO().UserData;

            // ------ DockSpace ------
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGuiWindowFlags host_window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_UnsavedDocument
                | ImGuiWindowFlags_NoResize;

            ImGui::Begin("Nova Editor", nullptr, host_window_flags);
            ImGui::PopStyleVar(3);

            ImGuiID dockspace_id = ImGui::GetID("NovaDockspace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

            setupDockSpace(useIni, dockspace_id);

            ImGui::End();

            // ------ Panels ------
            renderHierarchyPanel();
            renderViewportPanel(viewportTexture);
            renderInspectorPanel();
            renderAssetBrowserPanel();
        }
    
    } // namespace GUI
} // namespace Nova