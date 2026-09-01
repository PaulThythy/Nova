#include "Editor/EditorSelection.h"

#include <algorithm>
#include <iostream>
#include <string>

#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/NameComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "Scene/SceneQuery.h"

namespace Nova::App::Editor {

    using namespace Nova::Core::ECS::Components;
    using namespace Nova::Core::Math;

    void EditorSelection::SetSelected(entt::entity entity) {
        m_Entities.clear();
        if (entity != entt::null)
            m_Entities.push_back(entity);
    }

    void EditorSelection::Clear() {
        m_Entities.clear();
        ClearFocus();
    }

    void EditorSelection::ClearFocus() {
        m_FocusedEntity = entt::null;
        m_FocusInfo = {};
    }

    void EditorSelection::SetFocused(entt::entity entity, const glm::vec3& aabbCenter, const glm::vec3& aabbExtents, const std::string& name, uint32_t triangleCount) {
        m_FocusedEntity = entity;
        SetSelected(entity);

        m_FocusInfo.m_Entity = entity;
        m_FocusInfo.m_Name = name;
        m_FocusInfo.m_AabbCenter = aabbCenter;
        m_FocusInfo.m_AabbExtents = aabbExtents;
        m_FocusInfo.m_TriangleCount = triangleCount;
    }

    bool EditorSelection::IsSelected(entt::entity entity) const {
        return std::find(m_Entities.begin(), m_Entities.end(), entity) != m_Entities.end();
    }

    bool EditorSelection::ComputeEntityWorldAABB(Nova::Core::Scene::Scene& scene, entt::entity entity, AABB& outBounds) {
        auto& registry = scene.GetRegistry();
        auto* tc = registry.try_get<TransformComponent>(entity);
        auto* mc = registry.try_get<MeshComponent>(entity);
        if (!tc || !mc || !mc->m_AABBTree.IsBuilt())
            return false;

        const auto& nodes = mc->m_AABBTree.GetNodes();
        if (nodes.empty())
            return false;

        const AABB& local = nodes.front().m_Bounds;
        const glm::mat4 transform = tc->GetTransform();

        const glm::vec3 corners[8] = {
            { local.m_Min.x, local.m_Min.y, local.m_Min.z },
            { local.m_Max.x, local.m_Min.y, local.m_Min.z },
            { local.m_Min.x, local.m_Max.y, local.m_Min.z },
            { local.m_Max.x, local.m_Max.y, local.m_Min.z },
            { local.m_Min.x, local.m_Min.y, local.m_Max.z },
            { local.m_Max.x, local.m_Min.y, local.m_Max.z },
            { local.m_Min.x, local.m_Max.y, local.m_Max.z },
            { local.m_Max.x, local.m_Max.y, local.m_Max.z },
        };

        bool first = true;
        for (const glm::vec3& corner : corners) {
            const glm::vec3 world = glm::vec3(transform * glm::vec4(corner, 1.0f));
            if (first) {
                outBounds = AABB(world, world);
                first = false;
            } else {
                outBounds.Expand(world);
            }
        }
        return !first;
    }

    entt::entity EditorSelection::PickEntityAtViewportUV(Nova::Core::Scene::Scene& scene, const Camera& camera, float u, float v, AABB* outWorldAabb) {
        u = std::clamp(u, 0.0f, 1.0f);
        v = std::clamp(v, 0.0f, 1.0f);

        const Ray ray = Nova::Core::Scene::ScreenPointToRay(camera, u, v);
        Nova::Core::Scene::RaycastHit hit{};

        if (!Nova::Core::Scene::Raycast(scene, ray, hit))
            return entt::null;

        if (outWorldAabb)
            ComputeEntityWorldAABB(scene, hit.m_Entity, *outWorldAabb);

        return hit.m_Entity;
    }

    void EditorSelection::PickAtViewportUV(Nova::Core::Scene::Scene& scene, const Camera& camera, float u, float v, bool addToSelection) {
        ClearFocus();

        u = std::clamp(u, 0.0f, 1.0f);
        v = std::clamp(v, 0.0f, 1.0f);

        const Ray ray = Nova::Core::Scene::ScreenPointToRay(camera, u, v);
        Nova::Core::Scene::RaycastHit hit{};

        if (Nova::Core::Scene::Raycast(scene, ray, hit)) {
            if (addToSelection) {
                if (!IsSelected(hit.m_Entity))
                    m_Entities.push_back(hit.m_Entity);
            } else {
                SetSelected(hit.m_Entity);
            }
        } else {
            if (!addToSelection)
                Clear();
        }
    }

} // namespace Nova::App::Editor