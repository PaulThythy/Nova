#include "UI/Panels/InspectorPanel.h"

#include "imgui.h"

#include "ECS/Components/NameComponent.h"
#include "Editor/EditorLayer.h"
#include "UI/Components/LightComponentUI.h"
#include "UI/Components/TransformComponentUI.h"

namespace Nova::App::UI::Panels::InspectorPanel {

    using Nova::Core::ECS::Components::NameComponent;

    void Render(Nova::Core::Scene::Scene& scene, Editor::EditorLayer* editor) {
        ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoScrollbar);

        if (!editor || editor->GetSelection().Empty()) {
            ImGui::TextDisabled("No entity selected.");
            ImGui::End();
            return;
        }

        // Multi-selection: inspect / edit only the first selected entity for now.
        const entt::entity entity = editor->GetSelectedEntity();
        if (entity == entt::null || !scene.GetRegistry().valid(entity)) {
            ImGui::TextDisabled("No entity selected.");
            ImGui::End();
            return;
        }

        auto& registry = scene.GetRegistry();

        if (auto* name = registry.try_get<NameComponent>(entity))
            ImGui::TextUnformatted(name->m_Name.c_str());
        else
            ImGui::TextUnformatted("Unnamed");

        const auto& selected = editor->GetSelection().GetEntities();
        if (selected.size() > 1)
            ImGui::TextDisabled("%zu entities selected (editing first)", selected.size());

        ImGui::Separator();

        UI::Components::DrawTransformComponent(registry, entity);
        UI::Components::DrawLightComponent(registry, entity);

        ImGui::End();
    }

} // namespace Nova::App::UI::Panels::InspectorPanel