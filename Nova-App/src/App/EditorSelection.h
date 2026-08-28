#ifndef EDITORSELECTION_H
#define EDITORSELECTION_H

#include <vector>

#include <entt/entt.hpp>

#include "Math/Camera.h"
#include "Scene/Scene.h"

namespace Nova::App {

    class EditorSelection {
    public:
        void SetSelected(entt::entity entity);
        void Clear() { m_Entities.clear(); }
        bool IsSelected(entt::entity entity) const;

        entt::entity GetSelected() const {
            return m_Entities.empty() ? entt::null : m_Entities.front();
        }

        const std::vector<entt::entity>& GetEntities() const { return m_Entities; }
        std::vector<entt::entity>& GetEntities() { return m_Entities; }
        bool Empty() const { return m_Entities.empty(); }

        /** Viewport picking: UV in [0,1], (0,0) = top-left of the rendered image. */
        void PickAtViewportUV(
            Nova::Core::Scene::Scene& scene,
            const Nova::Core::Math::Camera& camera,
            float u,
            float v,
            bool addToSelection = false);

    private:
        std::vector<entt::entity> m_Entities;
    };

} // namespace Nova::App

#endif // EDITORSELECTION_H