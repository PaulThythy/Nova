#include "UI/Components/TransformComponentUI.h"

#include <glm/glm.hpp>

#include "imgui.h"

#include "ECS/Components/TransformComponent.h"

namespace Nova::App::UI::Components {

    using Nova::Core::ECS::Components::TransformComponent;

    constexpr float kAxisBarWidth = 3.0f;
    constexpr float kLabelColumnWidth = 90.0f;

    bool DrawAxisDragFloat(const char* id, float* value, const ImVec4& color, float width, float speed, const char* format) {
        ImGui::PushID(id);

        const ImVec2 pos = ImGui::GetCursorScreenPos();
        const float height = ImGui::GetFrameHeight();

        ImGui::GetWindowDrawList()->AddRectFilled(
            pos,
            ImVec2(pos.x + kAxisBarWidth, pos.y + height),
            ImGui::ColorConvertFloat4ToU32(color));

        ImGui::Dummy(ImVec2(kAxisBarWidth, height));
        ImGui::SameLine(0.0f, 0.0f);

        ImGui::PushItemWidth(width - kAxisBarWidth);
        const bool changed = ImGui::DragFloat("##v", value, speed, 0.0f, 0.0f, format);
        ImGui::PopItemWidth();

        ImGui::PopID();
        return changed;
    }

    bool DrawTRSvectors(const char* label, const char* id, glm::vec3& values, float speed, const char* format) {
        ImGui::PushID(id);

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        ImGui::SameLine(kLabelColumnWidth);

        const float avail = ImGui::GetContentRegionAvail().x;
        const float spacing = ImGui::GetStyle().ItemSpacing.x;
        const float inputW = (avail - 2.0f * spacing) / 3.0f;

        bool changed = false;
        changed |= DrawAxisDragFloat("x", &values.x, ImVec4(0.75f, 0.15f, 0.15f, 1.0f), inputW, speed, format);
        ImGui::SameLine();
        changed |= DrawAxisDragFloat("y", &values.y, ImVec4(0.20f, 0.60f, 0.20f, 1.0f), inputW, speed, format);
        ImGui::SameLine();
        changed |= DrawAxisDragFloat("z", &values.z, ImVec4(0.20f, 0.35f, 0.85f, 1.0f), inputW, speed, format);

        ImGui::PopID();
        return changed;
    }

    void DrawTransformComponent(entt::registry& registry, entt::entity entity) {
        auto* tc = registry.try_get<TransformComponent>(entity);
        if (!tc)
            return;

        if (!ImGui::CollapsingHeader("Transforms", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        DrawTRSvectors("Translation", "translation", tc->m_Translation, 0.01f, "%.2f");

        // Stored in radians on the component; edited as degrees in the UI.
        glm::vec3 rotationDegrees = glm::degrees(tc->m_Rotation);
        DrawTRSvectors("Rotation", "rotation", rotationDegrees, 1.0f, "%.2f\xc2\xb0");
        tc->m_Rotation = glm::radians(rotationDegrees);

        DrawTRSvectors("Scale", "scale", tc->m_Scale, 0.01f, "%.2f");

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Mobility");
        ImGui::SameLine(kLabelColumnWidth);

        const float mobilityAvail = ImGui::GetContentRegionAvail().x;
        const float buttonW = mobilityAvail / 3.0f;

        // TODO add mobility behaviour
        ImGui::BeginDisabled();
        ImGui::Button("Static", ImVec2(buttonW, 0.0f));
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::Button("Stationary", ImVec2(buttonW, 0.0f));
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::Button("Movable", ImVec2(buttonW, 0.0f));
        ImGui::EndDisabled();
    }

} // namespace Nova::App::UI::Components