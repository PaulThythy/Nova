#include "UI/Panels/HierarchyPanel.h"

#include <cstdint>
#include <vector>

#include "imgui.h"

#include "ECS/Components/NameComponent.h"
#include "Editor/EditorLayer.h"

namespace Nova::App::UI::Panels::HierarchyPanel {

    constexpr const char* kHierarchyPayload = "HIERARCHY_ENTITIES";

    using Nova::Core::ECS::Components::NameComponent;

    // True once a hierarchy drag has started; reset when the mouse button is released.
    bool s_HierarchyDragActive = false;

    const char* GetEntityLabel(Nova::Core::Scene::Scene& scene, entt::entity entity) {
        if (auto* name = scene.GetRegistry().try_get<NameComponent>(entity))
            return name->m_Name.c_str();
        return "Unnamed";
    }

    bool IsHierarchyDraggable(Nova::Core::Scene::Scene& scene, entt::entity entity) {
        return entity != entt::null
            && entity != scene.GetRootEntity()
            && entity != scene.GetMainCamera();
    }

    std::vector<entt::entity> CollectDragEntities(
        Nova::Core::Scene::Scene& scene,
        entt::entity entity,
        Editor::EditorSelection* selection) {
        std::vector<entt::entity> entities;

        if (selection && selection->IsSelected(entity) && selection->GetEntities().size() > 1) {
            for (entt::entity selected : selection->GetEntities()) {
                if (IsHierarchyDraggable(scene, selected))
                    entities.push_back(selected);
            }
        } else if (IsHierarchyDraggable(scene, entity)) {
            entities.push_back(entity);
        }

        return entities;
    }

    bool IsInList(const std::vector<entt::entity>& entities, entt::entity entity) {
        for (entt::entity e : entities) {
            if (e == entity)
                return true;
        }
        return false;
    }

    void ParentDraggedEntities(
        Nova::Core::Scene::Scene& scene,
        entt::entity newParent,
        const entt::entity* entities,
        std::size_t count) {
        if (!entities || count == 0 || newParent == entt::null)
            return;

        std::vector<entt::entity> dragged(entities, entities + count);

        for (entt::entity child : dragged) {
            if (child == newParent || !IsHierarchyDraggable(scene, child))
                continue;

            // Keep nested selection groups intact: only reparent selection roots.
            if (IsInList(dragged, scene.GetParent(child)))
                continue;

            scene.ParentEntity(child, newParent);
        }
    }

    void DrawEntityNode(
        Nova::Core::Scene::Scene& scene,
        entt::entity entity,
        Editor::EditorLayer* editor) {
        if (entity == scene.GetMainCamera())
            return;

        Editor::EditorSelection* selection = editor ? &editor->GetSelection() : nullptr;

        const auto& children = scene.GetChildren(entity);
        bool hasVisibleChildren = false;
        for (entt::entity child : children) {
            if (child != scene.GetMainCamera()) {
                hasVisibleChildren = true;
                break;
            }
        }

        const bool isRoot = entity == scene.GetRootEntity();
        const bool isSelected = selection && selection->IsSelected(entity);

        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth;

        if (isSelected)
            flags |= ImGuiTreeNodeFlags_Selected;
        if (isRoot)
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
        if (!hasVisibleChildren)
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

        const void* nodeId = reinterpret_cast<void*>(
            static_cast<uintptr_t>(entt::to_integral(entity)));
        const bool opened = ImGui::TreeNodeEx(nodeId, flags, "%s", GetEntityLabel(scene, entity));

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && editor) {
            editor->FocusEntity(entity);
        } else if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen() && selection) {
            if (ImGui::GetIO().KeyShift)
                selection->AddSelected(entity);
            else if (!isSelected)
                // Keep multi-selection when re-clicking an already selected entity
                // so drag-and-drop can move the whole group.
                selection->SetSelected(entity);
        }

        // Click (no drag) on one item of a multi-selection → select only that item.
        if (selection
            && isSelected
            && !ImGui::GetIO().KeyShift
            && selection->GetEntities().size() > 1
            && ImGui::IsItemHovered()
            && ImGui::IsMouseReleased(ImGuiMouseButton_Left)
            && !s_HierarchyDragActive) {
            selection->SetSelected(entity);
        }

        if (IsHierarchyDraggable(scene, entity) && ImGui::BeginDragDropSource()) {
            s_HierarchyDragActive = true;
            const std::vector<entt::entity> dragEntities = CollectDragEntities(scene, entity, selection);
            if (!dragEntities.empty()) {
                ImGui::SetDragDropPayload(
                    kHierarchyPayload,
                    dragEntities.data(),
                    dragEntities.size() * sizeof(entt::entity));

                if (dragEntities.size() == 1)
                    ImGui::TextUnformatted(GetEntityLabel(scene, dragEntities.front()));
                else
                    ImGui::Text("%zu entities", dragEntities.size());
            }
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kHierarchyPayload)) {
                const auto* entities = static_cast<const entt::entity*>(payload->Data);
                const std::size_t count = payload->DataSize / sizeof(entt::entity);
                ParentDraggedEntities(scene, entity, entities, count);
            }
            ImGui::EndDragDropTarget();
        }

        if (opened && hasVisibleChildren) {
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

        if (editor
            && ImGui::IsWindowHovered()
            && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
            && !ImGui::IsAnyItemHovered()) {
            editor->ClearSelection();
        }

        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
            s_HierarchyDragActive = false;

        ImGui::End();
    }

} // namespace Nova::App::UI::Panels::HierarchyPanel