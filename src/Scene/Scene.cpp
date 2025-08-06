#include "Scene/Scene.hpp"

namespace Nova {

    std::optional<Scene::RayHitEntity> Scene::pickEntity(const Math::Ray& rayWorld, entt::registry&  reg) {
        std::optional<Scene::RayHitEntity> best;

        auto view = reg.view<Nova::Components::MeshComponent, Nova::Components::TransformComponent>();

        glm::vec3 dirW = glm::normalize(rayWorld.m_Direction);

        for (auto e : view)
        {
            const auto& tf   = view.get<Nova::Components::TransformComponent>(e);
            const auto& mesh = view.get<Nova::Components::MeshComponent>(e);

            // Transform to object space
            glm::mat4 model = tf.GetTransform();
            glm::mat4 invModel = glm::inverse(model);
            glm::vec3 origObj  = glm::vec3(invModel * glm::vec4(rayWorld.m_Origin,1));
            glm::vec3 dirObj   = glm::normalize(glm::vec3(invModel * glm::vec4(dirW,0)));
            Math::Ray ray(origObj, dirObj);

            float closestT = std::numeric_limits<float>::max();

            for(size_t i=0;i<mesh.m_Indices.size();i+=3)
            {
                const glm::vec3& v0 = mesh.m_Vertices[ mesh.m_Indices[i]   ];
                const glm::vec3& v1 = mesh.m_Vertices[ mesh.m_Indices[i+1] ];
                const glm::vec3& v2 = mesh.m_Vertices[ mesh.m_Indices[i+2] ];
                float t;
                if(Math::rayTriangle(ray, v0,v1,v2, t))
                    if (t > 0.0f && t < closestT) {
                        closestT = t;
                    }
            }

            if (closestT == std::numeric_limits<float>::max())
                continue;

            glm::vec3 hitObj   = origObj + closestT * dirObj;
            glm::vec3 hitWorld = glm::vec3(model * glm::vec4(hitObj, 1.0f));

            float tWorld = glm::dot(hitWorld - rayWorld.m_Origin, dirW); // dirW normalisé
            if (tWorld > 0.0f) {
                if (!best || tWorld < best->distance) {
                    best = Scene::RayHitEntity{ e, tWorld };
                }
            }
        }
        return best;
    }

} // namespace Nova