#include "UI/Panels/HierarchyPanel.h"

#include <cstdint>

#include "imgui.h"

#include "ECS/Components/NameComponent.h"
#include "Editor/EditorLayer.h"

namespace Nova::App::UI::Panels::HierarchyPanel {

    using Nova::Core::ECS::Components::NameComponent;

    const char* GetEntityLabel(Nova::Core::Scene::Scene& scene, entt::entity entity) {
        if (auto* name = scene.GetRegistry().try_get<NameComponent>(entity))
            return name->m_Name.c_str();
        return "Unnamed";
    }

    void DrawEntityNode(
        Nova::Core::Scene::Scene& scene,
        entt::entity entity,
        Editor::EditorLayer* editor) {
        Editor::EditorSelection* selection = editor ? &editor->GetSelection() : nullptr;

        const auto& children = scene.GetChildren(entity);
        const bool hasChildren = !children.empty();
        const bool isRoot = entity == scene.GetRootEntity();
        const bool isSelected = selection && selection->IsSelected(entity);

        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth;

        if (isSelected)
            flags |= ImGuiTreeNodeFlags_Selected;
        if (isRoot)
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
        if (!hasChildren)
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

        const void* nodeId = reinterpret_cast<void*>(
            static_cast<uintptr_t>(entt::to_integral(entity)));
        const bool opened = ImGui::TreeNodeEx(nodeId, flags, "%s", GetEntityLabel(scene, entity));

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && editor) {
            editor->FocusEntity(entity);
        } else if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen() && selection) {
            if (ImGui::GetIO().KeyShift)
                selection->AddSelected(entity);
            else
                selection->SetSelected(entity);
        }

        if (opened && hasChildren) {
            for (entt::entity child : children)
                DrawEntityNode(scene, child, editor);
            ImGui::TreePop();
        }
    }

    void Render(Nova::Core::Scene::Scene& scene, Editor::EditorLayer* editor) {
        ImGui::Begin("Hierarchy");

        const entt::entity root = scene.GetRootEntity();
        if (root != entt::null)
            DrawEntityNode(scene, root, editor);

        ImGui::End();
    }

} // namespace Nova::App::UI::Panels::HierarchyPanel