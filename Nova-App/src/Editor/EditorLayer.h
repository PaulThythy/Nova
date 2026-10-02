#ifndef EDITORLAYER_H
#define EDITORLAYER_H

#include <vector>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include "Editor/EditorSelection.h"
#include "Core/Layer.h"
#include "Events/Event.h"
#include "Events/InputEvents.h"

namespace Nova::App {
    class AppLayer;
} // namespace Nova::App

namespace Nova::App::Editor {

    enum class GizmoOperation {
        Translate = 0,
        Rotate,
        Scale,
    };

    enum class GizmoSpace {
        Local = 0,
        World,
    };

    class EditorLayer : public Nova::Core::Layer {
    public:
        explicit EditorLayer(): Layer("EditorLayer") {}
        ~EditorLayer() override = default;

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(float dt) override;
        void OnBegin() override;
        void OnRender() override;
        void OnEnd() override;
        void OnImGuiRender() override;
        void OnEvent(Nova::Core::Events::Event& e) override;

        EditorSelection& GetSelection() { return m_Selection; }
        const EditorSelection& GetSelection() const { return m_Selection; }

        void PickAtViewportUV(float u, float v, bool addToSelection = false);
        void FocusAtViewportUV(float u, float v);
        void FocusEntity(entt::entity entity);
        void ClearFocus();

        entt::entity GetFocusedEntity() const { return m_Selection.GetFocused(); }
        bool HasFocus() const { return m_Selection.HasFocus(); }
        const FocusInfo& GetFocusInfo() const { return m_Selection.GetFocusInfo(); }

        const std::vector<entt::entity>& GetSelectedEntities() const {
            return m_Selection.GetEntities();
        }
        entt::entity GetSelectedEntity() const { return m_Selection.GetSelected(); }
        void SetSelectedEntity(entt::entity entity) { m_Selection.SetSelected(entity); }
        void ClearSelection() { m_Selection.Clear(); }
        bool IsSelected(entt::entity entity) const { return m_Selection.IsSelected(entity); }
        GizmoOperation GetGizmoOperation() const { return m_GizmoOperation; }
        void SetGizmoOperation(GizmoOperation operation) { m_GizmoOperation = operation; }
        GizmoSpace GetGizmoSpace() const { return m_GizmoSpace; }
        void SetGizmoSpace(GizmoSpace space) { m_GizmoSpace = space; }
        bool IsGizmoDragging() const { return m_IsGizmoDragging; }
        bool ShouldBlockViewportPick() const { return m_BlockViewportPick; }
        void SetGizmoDragging(bool isDragging);
        void ConsumeViewportPickBlock() { m_BlockViewportPick = false; }

    private:
        bool OnKeyPressed(Nova::Core::Events::KeyPressedEvent& e);

        EditorSelection m_Selection;
        GizmoOperation m_GizmoOperation{ GizmoOperation::Translate };
        GizmoSpace m_GizmoSpace{ GizmoSpace::Local };
        bool m_IsGizmoDragging{ false };
        bool m_BlockViewportPick{ false };
    };

} // namespace Nova::App::Editor

#endif // EDITORLAYER_H