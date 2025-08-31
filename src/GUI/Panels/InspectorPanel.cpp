#include "GUI/Panels/InspectorPanel.hpp"
#include "imgui.h"
#include "Components/MeshRendererComponent.hpp"
#include "Components/TransformComponent.hpp"
#include "Components/LightComponent.hpp"

namespace Nova::GUI::InspectorPanel {

    void DrawTRSvectors(const char* label, const char* xButtonLabel, const char* xInputLabel,
                        const char* yButtonLabel, const char* yInputLabel,
                        const char* zButtonLabel, const char* zInputLabel,
                        glm::vec3& values, float inputW, float badgeW, float groupSpacing) {
        ImGui::Text("%s", label);
        ImGui::Spacing();

        ImGuiIO& io = ImGui::GetIO();
        const float speed = 0.1f;
        //TODO add snaping
        //const float speed = base * (io.KeyShift ? 2.0f : (io.KeyAlt ? 0.1f : 1.0f));

        // X
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1,0,0,1));
        ImGui::BeginDisabled();
        ImGui::Button(xButtonLabel, ImVec2(badgeW,0));
        ImGui::EndDisabled();
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PushItemWidth(inputW);
        ImGui::DragFloat(xInputLabel, &values.x, speed, 0.0f, 0.0f, "%.6f");
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
        ImGui::DragFloat(yInputLabel, &values.y, speed, 0.0f, 0.0f, "%.6f");
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
        ImGui::DragFloat(zInputLabel, &values.z, speed, 0.0f, 0.0f, "%.6f");
        ImGui::PopItemWidth();
    }

    void DrawTransform(entt::registry& reg, entt::entity e, float inputW, float badgeW, float groupSpacing) {
        if (!reg.any_of<Nova::Components::TransformComponent>(e)) return;
        if(ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {

            DrawTRSvectors("Position", "X###posXBadge", "##posX", "Y###posYBadge", "##posY", "Z###posZBadge", "##posZ", reg.get<Nova::Components::TransformComponent>(e).m_Position, inputW, badgeW, groupSpacing);
            DrawTRSvectors("Rotation", "X###rotXBadge", "##rotX", "Y###rotYBadge", "##rotY", "Z###rotZBadge", "##rotZ", reg.get<Nova::Components::TransformComponent>(e).m_Rotation, inputW, badgeW, groupSpacing);
            DrawTRSvectors("Scale", "X###scaleXBadge", "##scaleX", "Y###scaleYBadge", "##scaleY", "Z###scaleZBadge", "##scaleZ", reg.get<Nova::Components::TransformComponent>(e).m_Scale, inputW, badgeW, groupSpacing);
        }
    }

    void DrawMeshRenderer(entt::registry& reg, entt::entity e) {
        if (auto* mr = reg.try_get<Components::MeshRendererComponent>(e)) {
            if(ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox("Visible", &mr->m_Visible);
                ImGui::SameLine();
                ImGui::Checkbox("Wireframe", &mr->m_Wireframe);

                ImGui::Checkbox("Cast Shadows", &mr->m_CastShadows);

                ImGui::Separator();

                ImGui::ColorEdit3("Base Color", &mr->m_BaseColor.x);
                ImGui::SliderFloat("Roughness", &mr->m_Roughness, 0.0f, 1.0f);
                ImGui::SliderFloat("Metallic",  &mr->m_Metallic,  0.0f, 1.0f);

                ImGui::Separator();

                ImGui::ColorEdit3("Emissive Color", &mr->m_EmissiveColor.x);
                ImGui::SliderFloat("Emissive Strength", &mr->m_EmissiveStrength, 0.0f, 1.0f);
            }
        }
    }

    void DrawLightComponent(entt::registry& reg, entt::entity e) {
        if (auto* light = reg.try_get<Components::LightComponent>(e)) {
            if(ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                
                // Type selection
                const char* items[] = { "Directional", "Spot", "Point" };
                int currentType = static_cast<int>(light->m_Type);
                if (ImGui::Combo("Type", &currentType, items, IM_ARRAYSIZE(items))) {
                    light->m_Type = static_cast<Components::LightType>(currentType);
                }
                ImGui::Separator();

                // Common parameters
                ImGui::ColorEdit3("Color", &light->m_Color.x);
                ImGui::SliderFloat("Intensity", &light->m_Intensity, 0.0f, 5.0f);
                ImGui::Checkbox("Cast Shadows", &light->m_LightShadows);
                ImGui::Separator();

                // Type-specific parameters
                switch (light->m_Type) {
                    case Components::LightType::Directional:
                        // No specific parameters to display
                        break;
                    case Components::LightType::Point:
                        ImGui::SliderFloat("Range", &light->m_Range, 0.1f, 100.0f);
                        break;
                    case Components::LightType::Spot:
                        ImGui::SliderFloat("Range", &light->m_Range, 0.1f, 100.0f);
                        ImGui::SliderFloat("Inner Cone (deg)", &light->m_InnerCone, 0.0f, 180.0f);
                        ImGui::SliderFloat("Outer Cone (deg)", &light->m_OuterCone, 0.0f, 180.0f);
                        if (light->m_InnerCone > light->m_OuterCone) {
                            light->m_OuterCone = light->m_InnerCone;
                        }
                        break;
                }
            }
        }
    }

    void Render(Nova::Scene& scene) {
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

        if (!scene.HasSelection()) {
            ImGui::TextDisabled("No selection");
            ImGui::End();
            return;
        }

        auto selectedEntities = scene.GetSelected();
        entt::entity selectedEntity = *selectedEntities.begin();            //take first element
        auto& reg = scene.Registry();

        DrawTransform(reg, selectedEntity, inputW, badgeW, groupSpacing);
        DrawMeshRenderer(reg, selectedEntity);
        DrawLightComponent(reg, selectedEntity);

        ImGui::End();
    }

} // namespace Nova::GUI