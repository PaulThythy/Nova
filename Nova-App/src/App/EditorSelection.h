#ifndef EDITORSELECTION_H
#define EDITORSELECTION_H

#include <optional>
#include <vector>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include "Core/Log.h"
#include "Math/AABB.h"
#include "Math/Camera.h"
#include "Scene/Scene.h"

namespace Nova::App {

    struct FocusInfo {
        entt::entity m_Entity{ entt::null };
        std::string m_Name;
        glm::vec3 m_AabbCenter{0.0f};
        glm::vec3 m_AabbExtents{0.0f};
        uint32_t m_TriangleCount = 0;
    };

    class EditorSelection {
    public:
        void SetSelected(entt::entity entity);
        void Clear();
        bool IsSelected(entt::entity entity) const;

        entt::entity GetSelected() const {
            return m_Entities.empty() ? entt::null : m_Entities.front();
        }

        const std::vector<entt::entity>& GetEntities() const { return m_Entities; }
        std::vector<entt::entity>& GetEntities() { return m_Entities; }
        bool Empty() const { return m_Entities.empty(); }

        entt::entity GetFocused() const { return m_FocusedEntity; }
        bool HasFocus() const { return m_FocusedEntity != entt::null; }
        std::optional<FocusInfo> GetFocusInfo() const { return m_FocusInfo; }

        void ClearFocus();
        void SetFocused(entt::entity entity, const glm::vec3& aabbCenter, const glm::vec3& aabbExtents, const std::string& name, uint32_t triangleCount = 0);

        /** Viewport picking: UV in [0,1], (0,0) = top-left of the rendered image. */
        void PickAtViewportUV(Nova::Core::Scene::Scene& scene, const Nova::Core::Math::Camera& camera, float u, float v, bool addToSelection = false);

        /** Returns picked entity, or entt::null. Optionally writes world AABB. */
        static entt::entity PickEntityAtViewportUV(Nova::Core::Scene::Scene& scene, const Nova::Core::Math::Camera& camera, float u, float v, Nova::Core::Math::AABB* outWorldAabb = nullptr);

        static bool ComputeEntityWorldAABB(Nova::Core::Scene::Scene& scene, entt::entity entity, Nova::Core::Math::AABB& outBounds);

    private:
        std::vector<entt::entity> m_Entities;
        entt::entity m_FocusedEntity{ entt::null };
        std::optional<FocusInfo> m_FocusInfo;
    };

} // namespace Nova::App

#endif // EDITORSELECTION_H