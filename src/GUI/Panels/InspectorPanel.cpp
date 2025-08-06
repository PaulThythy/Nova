#include "GUI/Panels/InspectorPanel.hpp"
#include "imgui.h"

namespace Nova::GUI {

    void drawTRSvectors(const char* label, const char* xButtonLabel, const char* xInputLabel,
                        const char* yButtonLabel, const char* yInputLabel,
                        const char* zButtonLabel, const char* zInputLabel,
                        glm::vec3& values, float inputW, float badgeW, float groupSpacing) {
        ImGui::Text("%s", label);
        ImGui::Spacing();

        // X
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1,0,0,1));
        ImGui::BeginDisabled();
        ImGui::Button(xButtonLabel, ImVec2(badgeW,0));
        ImGui::EndDisabled();
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PushItemWidth(inputW);
        ImGui::InputFloat(xInputLabel, &values.x, 0.0f, 0.0f, "%.6f");
        ImGui::PopItemWidth();
        ImGui::SameLine(0, groupSpacing);

        // Y
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,1,0,1));
        ImGui::BeginDisabled();
        ImGui::Button(yButtonLabel, ImVec2(badgeW,0));
        ImGui::EndDisabled();
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PushItemWidth(inputW);
        ImGui::InputFloat(yInputLabel, &values.y, 0.0f, 0.0f, "%.6f");
        ImGui::PopItemWidth();
        ImGui::SameLine(0, groupSpacing);

        // Z
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,1,1));
        ImGui::BeginDisabled();
        ImGui::Button(zButtonLabel, ImVec2(badgeW,0));
        ImGui::EndDisabled();
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PushItemWidth(inputW);
        ImGui::InputFloat(zInputLabel, &values.z, 0.0f, 0.0f, "%.6f");
        ImGui::PopItemWidth();
    }

    void drawTransform(entt::registry& reg, entt::entity e, float inputW, float badgeW, float groupSpacing) {
        if (!reg.any_of<Nova::Components::TransformComponent>(e)) return;
        if(ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {

            drawTRSvectors("Position", "X###posBadge", "##posX", "Y###posBadge", "##posY", "Z###posBadge", "##posZ", reg.get<Nova::Components::TransformComponent>(e).m_Position, inputW, badgeW, groupSpacing);
            drawTRSvectors("Rotation", "X###rotBadge", "##rotX", "Y###rotBadge", "##rotY", "Z###rotBadge", "##rotZ", reg.get<Nova::Components::TransformComponent>(e).m_Rotation, inputW, badgeW, groupSpacing);
            drawTRSvectors("Scale", "X###scaleBadge", "##scaleX", "Y###scaleBadge", "##scaleY", "Z###scaleBadge", "##scaleZ", reg.get<Nova::Components::TransformComponent>(e).m_Scale, inputW, badgeW, groupSpacing);
        }
    }

    void renderInspectorPanel(Nova::Scene& scene) {
        ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

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

        if (!scene.hasSelection()) {
            ImGui::TextDisabled("No selection");
            ImGui::End();
            return;
        }

        auto e = scene.getSelected();
        auto& reg = scene.registry();

        drawTransform(reg, e, inputW, badgeW, groupSpacing);

        ImGui::End();
    }

} // namespace Nova::GUI