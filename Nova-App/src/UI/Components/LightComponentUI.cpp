#include "UI/Components/LightComponentUI.h"

#include <algorithm>

#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"

#include "ECS/Components/LightComponent.h"
#include "Math/Light.h"

namespace Nova::App::UI::Components {

    using Nova::Core::ECS::Components::LightComponent;
    using Nova::Core::Math::Light;
    using Nova::Core::Math::LightType;

    constexpr float kLabelColumnWidth = 120.0f;

    static void DrawLabeledColorEdit(const char* label, const char* id, glm::vec3& color) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        ImGui::SameLine(kLabelColumnWidth);
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::ColorEdit3(id, glm::value_ptr(color), ImGuiColorEditFlags_NoLabel);
    }

    static void DrawLabeledDragFloat(const char* label, const char* id, float* value, float speed, float minV, float maxV, const char* format) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        ImGui::SameLine(kLabelColumnWidth);
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::DragFloat(id, value, speed, minV, maxV, format);
    }

    void DrawLightComponent(entt::registry& registry, entt::entity entity) {
        auto* lc = registry.try_get<LightComponent>(entity);
        if (!lc || !lc->m_Light)
            return;

        if (!ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        Light& light = *lc->m_Light;

        static const char* kTypeNames[] = { "Directional", "Point", "Spot" };
        int typeIndex = static_cast<int>(light.m_Type);
        typeIndex = std::clamp(typeIndex, 0, 2);

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Type");
        ImGui::SameLine(kLabelColumnWidth);
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::Combo("##LightType", &typeIndex, kTypeNames, IM_ARRAYSIZE(kTypeNames)))
            light.m_Type = static_cast<LightType>(typeIndex);

        DrawLabeledColorEdit("Color", "##LightColor", light.m_Color);
        DrawLabeledDragFloat("Intensity", "##LightIntensity", &light.m_Intensity, 0.05f, 0.0f, 0.0f, "%.2f");
        if (light.m_Intensity < 0.0f)
            light.m_Intensity = 0.0f;

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Cast Shadows");
        ImGui::SameLine(kLabelColumnWidth);
        ImGui::Checkbox("##LightShadow", &light.m_LightShadow);

        if (light.m_Type == LightType::Point || light.m_Type == LightType::Spot) {
            DrawLabeledDragFloat("Range", "##LightRange", &light.m_Range, 0.1f, 0.01f, 0.0f, "%.2f");
            if (light.m_Range < 0.01f)
                light.m_Range = 0.01f;
        }

        if (light.m_Type == LightType::Spot) {
            DrawLabeledDragFloat("Inner Cone", "##LightInnerCone", &light.m_InnerCone, 0.25f, 0.0f, 89.0f, "%.1f°");
            DrawLabeledDragFloat("Outer Cone", "##LightOuterCone", &light.m_OuterCone, 0.25f, 0.0f, 89.0f, "%.1f°");

            light.m_InnerCone = std::clamp(light.m_InnerCone, 0.0f, 89.0f);
            light.m_OuterCone = std::clamp(light.m_OuterCone, 0.0f, 89.0f);
            if (light.m_OuterCone < light.m_InnerCone)
                light.m_OuterCone = light.m_InnerCone;
        }

        if (light.m_LightShadow && (light.m_Type == LightType::Directional || light.m_Type == LightType::Spot)) {
            ImGui::Separator();
            ImGui::TextDisabled("Shadow Bias");
            DrawLabeledDragFloat("Constant", "##ShadowBiasConstant", &light.m_ShadowBiasConstant, 0.01f, 0.0f, 0.0f, "%.3f");
            DrawLabeledDragFloat("Slope", "##ShadowBiasSlope", &light.m_ShadowBiasSlope, 0.01f, 0.0f, 0.0f, "%.3f");
            DrawLabeledDragFloat("Normal", "##ShadowNormalBias", &light.m_ShadowNormalBias, 0.001f, 0.0f, 0.0f, "%.4f");
        }
    }

} // namespace Nova::App::UI::Components