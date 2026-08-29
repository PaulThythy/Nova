#ifndef EDITORLAYER_H
#define EDITORLAYER_H

#include <optional>
#include <vector>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include "App/EditorSelection.h"
#include "Core/Layer.h"
#include "Events/Event.h"
#include "Events/InputEvents.h"

namespace Nova::App {

    class AppLayer;

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
        void ClearFocus();

        entt::entity GetFocusedEntity() const { return m_Selection.GetFocused(); }
        bool HasFocus() const { return m_Selection.HasFocus(); }
        std::optional<FocusInfo> GetFocusInfo() const { return m_Selection.GetFocusInfo(); }

        const std::vector<entt::entity>& GetSelectedEntities() const {
            return m_Selection.GetEntities();
        }
        entt::entity GetSelectedEntity() const { return m_Selection.GetSelected(); }
        void SetSelectedEntity(entt::entity entity) { m_Selection.SetSelected(entity); }
        void ClearSelection() { m_Selection.Clear(); }
        bool IsSelected(entt::entity entity) const { return m_Selection.IsSelected(entity); }

    private:
        bool OnKeyPressed(Nova::Core::Events::KeyPressedEvent& e);

        EditorSelection m_Selection;
    };

} // namespace Nova::App

#endif // EDITORLAYER_H