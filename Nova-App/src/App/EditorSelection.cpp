#include "App/EditorSelection.h"

#include <algorithm>
#include <iostream>

#include "ECS/Components/NameComponent.h"
#include "Scene/SceneQuery.h"

namespace Nova::App {

    void EditorSelection::SetSelected(entt::entity entity) {
        m_Entities.clear();
        if (entity != entt::null)
            m_Entities.push_back(entity);
    }

    bool EditorSelection::IsSelected(entt::entity entity) const {
        return std::find(m_Entities.begin(), m_Entities.end(), entity) != m_Entities.end();
    }

    void EditorSelection::PickAtViewportUV(
        Nova::Core::Scene::Scene& scene,
        const Nova::Core::Math::Camera& camera,
        float u,
        float v,
        bool addToSelection)
    {
        u = std::clamp(u, 0.0f, 1.0f);
        v = std::clamp(v, 0.0f, 1.0f);

        const Nova::Core::Math::Ray ray = Nova::Core::Scene::ScreenPointToRay(camera, u, v);
        Nova::Core::Scene::RaycastHit hit{};

        if (Nova::Core::Scene::Raycast(scene, ray, hit)) {
            if (addToSelection) {
                if (!IsSelected(hit.m_Entity))
                    m_Entities.push_back(hit.m_Entity);
            } else {
                SetSelected(hit.m_Entity);
            }

            std::string name = "unnamed";
            if (auto* nc = scene.GetRegistry().try_get<Nova::Core::ECS::Components::NameComponent>(hit.m_Entity))
                name = nc->m_Name;

            std::cout << "[Pick] Selected \"" << name
                      << "\" (count=" << m_Entities.size()
                      << ") dist=" << hit.m_Distance
                      << " tri=" << hit.m_TriangleIndex << '\n';
        } else {
            if (!addToSelection)
                Clear();
            std::cout << "[Pick] Nothing hit\n";
        }
    }

} // namespace Nova::App