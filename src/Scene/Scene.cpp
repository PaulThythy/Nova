#include "Scene/Scene.hpp"

namespace Nova {

    std::optional<Scene::RayHitEntity> Scene::pickEntity(const Math::Ray& rayWorld, entt::registry&  reg) {
        std::optional<Scene::RayHitEntity> best;

        auto view = reg.view<Nova::Components::MeshComponent, Nova::Components::TransformComponent>();

        for (auto e : view)
        {
            const auto& tf   = view.get<Nova::Components::TransformComponent>(e);
            const auto& mesh = view.get<Nova::Components::MeshComponent>(e);

            // Transform to object space
            glm::mat4 invModel = glm::inverse(tf.GetTransform());
            glm::vec3 origObj  = glm::vec3(invModel * glm::vec4(rayWorld.m_Origin,1));
            glm::vec3 dirObj   = glm::normalize(glm::vec3(invModel * glm::vec4(rayWorld.m_Direction,0)));
            Math::Ray ray(origObj, dirObj);

            float closestT = std::numeric_limits<float>::max();
            for(size_t i=0;i<mesh.m_Indices.size();i+=3)
            {
                const glm::vec3& v0 = mesh.m_Vertices[ mesh.m_Indices[i]   ];
                const glm::vec3& v1 = mesh.m_Vertices[ mesh.m_Indices[i+1] ];
                const glm::vec3& v2 = mesh.m_Vertices[ mesh.m_Indices[i+2] ];
                float t;
                if(Math::rayTriangle(ray, v0,v1,v2, t))
                    if(t < closestT) closestT = t;
            }

            if(closestT != std::numeric_limits<float>::max())
            {
                if(!best || closestT < best->distance)
                    best = Scene::RayHitEntity{ e, closestT };
            }
        }
        return best;
    }

} // namespace Nova