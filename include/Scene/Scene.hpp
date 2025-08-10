#ifndef SCENE_HPP
#define SCENE_HPP

#include <entt/entt.hpp>
#include <optional>

#include "Components/TagComponent.hpp"
#include "Components/CameraComponent.hpp"
#include "Components/TransformComponent.hpp"
#include "Components/MeshComponent.hpp"
#include "Components/MeshRendererComponent.hpp"
#include "Math/Ray.hpp"

namespace Nova {

    class Scene {
    public:
        Scene() = default;
        ~Scene() = default;
    
        entt::registry& registry() { return m_Registry; }
        const entt::registry& registry() const { return m_Registry; }

        struct RayHitEntity {
            entt::entity  entity;
            float         distance;
        };

        std::optional<RayHitEntity> pickEntity(const Math::Ray& rayWorld, entt::registry&  reg);

        entt::entity createEntity(const std::string& tag = "") {
            entt::entity entity = m_Registry.create();
            if (!tag.empty()) {
                m_Registry.emplace<Nova::Components::TagComponent>(entity, tag);
            }
            return entity;
        }

        entt::entity getViewportCamera() const { return m_ViewportCamera; }

        entt::entity createViewportCamera(const std::string& tag = "ViewportCamera") {
            entt::entity cam = createEntity(tag);
            m_Registry.emplace<Nova::Components::CameraComponent>(cam);
            auto &camera = registry().get<Nova::Components::CameraComponent>(cam);
            camera.m_IsViewportCamera= true;
            m_ViewportCamera = cam;
            return cam;
        }

        template<typename... Comps, typename Func>
        void forEach(Func&& system) {
            m_Registry.view<Comps...>().each(std::forward<Func>(system));
        }

        void addToSelection(entt::entity e)      {m_Selected.insert(e);}
        void removeFromSelection(entt::entity e) { m_Selected.erase(e); }
        void clearSelection()                    { m_Selected.clear(); }
        bool isSelected(entt::entity e) const    { return m_Selected.count(e) > 0; }
        bool hasSelection() const               { return !m_Selected.empty(); }
        void destroyEntity(entt::entity e) {
            if (isSelected(e)) removeFromSelection(e);
            if (m_Registry.valid(e)) m_Registry.destroy(e);
        }
        const std::unordered_set<entt::entity>& getSelected() const { return m_Selected; }

    private:

        entt::registry m_Registry;
        entt::entity m_ViewportCamera = entt::null;
        std::unordered_set<entt::entity> m_Selected;
    };
    
} // namespace Nova

#endif // SCENE_HPP