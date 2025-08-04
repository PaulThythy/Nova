#include "GUI/Panels/InspectorPanel.hpp"
#include "imgui.h"

namespace Nova::GUI {

    void renderInspectorPanel() {
        ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
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
                ImGui::BeginDisabled();
                ImGui::Button("X##posBadge", ImVec2(badgeW,0));
                ImGui::EndDisabled();
                ImGui::PopStyleColor();
                ImGui::SameLine();
                ImGui::PushItemWidth(inputW);
                ImGui::InputFloat("##posX", &position[0], 0.0f, 0.0f, "%.6f");
                ImGui::PopItemWidth();
                ImGui::SameLine(0, groupSpacing);

                // Y
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,1,0,1));
                ImGui::BeginDisabled();
                ImGui::Button("Y##posBadge", ImVec2(badgeW,0));
                ImGui::EndDisabled();
                ImGui::PopStyleColor();
                ImGui::SameLine();
                ImGui::PushItemWidth(inputW);
                ImGui::InputFloat("##posY", &position[1], 0.0f, 0.0f, "%.6f");
                ImGui::PopItemWidth();
                ImGui::SameLine(0, groupSpacing);

                // Z
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,1,1));
                ImGui::BeginDisabled();
                ImGui::Button("Z##posBadge", ImVec2(badgeW,0));
                ImGui::EndDisabled();
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
                ImGui::BeginDisabled();
                ImGui::Button("X##rotBadge", ImVec2(badgeW,0));
                ImGui::EndDisabled();
                ImGui::PopStyleColor();
                ImGui::SameLine();
                ImGui::PushItemWidth(inputW);
                ImGui::InputFloat("##rotX", &rotation[0], 0.0f, 0.0f, "%.6f");
                ImGui::PopItemWidth();
                ImGui::SameLine(0, groupSpacing);

                // Y
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,1,0,1));
                ImGui::BeginDisabled();
                ImGui::Button("Y##rotBadge", ImVec2(badgeW,0));
                ImGui::EndDisabled();
                ImGui::PopStyleColor();
                ImGui::SameLine();
                ImGui::PushItemWidth(inputW);
                ImGui::InputFloat("##rotY", &rotation[1], 0.0f, 0.0f, "%.6f");
                ImGui::PopItemWidth();
                ImGui::SameLine(0, groupSpacing);

                // Z
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,1,1));
                ImGui::BeginDisabled();
                ImGui::Button("Z##rotBadge", ImVec2(badgeW,0));
                ImGui::EndDisabled();
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
                ImGui::BeginDisabled();
                ImGui::Button("X##sclBadge", ImVec2(badgeW,0));
                ImGui::EndDisabled();
                ImGui::PopStyleColor();
                ImGui::SameLine();
                ImGui::PushItemWidth(inputW);
                ImGui::InputFloat("##sclX", &scale[0], 0.0f, 0.0f, "%.6f");
                ImGui::PopItemWidth();
                ImGui::SameLine(0, groupSpacing);

                // Y
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,1,0,1));
                ImGui::BeginDisabled();
                ImGui::Button("Y##sclBadge", ImVec2(badgeW,0));
                ImGui::EndDisabled();
                ImGui::PopStyleColor();
                ImGui::SameLine();
                ImGui::PushItemWidth(inputW);
                ImGui::InputFloat("##sclY", &scale[1], 0.0f, 0.0f, "%.6f");
                ImGui::PopItemWidth();
                ImGui::SameLine(0, groupSpacing);

                // Z
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,1,1));
                ImGui::BeginDisabled();
                ImGui::Button("Z##sclBadge", ImVec2(badgeW,0));
                ImGui::EndDisabled();
                ImGui::PopStyleColor();
                ImGui::SameLine();
                ImGui::PushItemWidth(inputW);
                ImGui::InputFloat("##sclZ", &scale[2], 0.0f, 0.0f, "%.6f");
                ImGui::PopItemWidth();
            }
        }
        ImGui::End();
    }

} // namespace Nova::GUI