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
    
        entt::registry& Registry() { return m_Registry; }
        const entt::registry& Registry() const { return m_Registry; }

        struct RayHitEntity {
            entt::entity  m_Entity;
            float         m_Distance;
        };

        std::optional<RayHitEntity> PickEntity(const Math::Ray& rayWorld, entt::registry&  reg);

        entt::entity CreateEntity(const std::string& tag = "") {
            entt::entity entity = m_Registry.create();
            if (!tag.empty()) {
                m_Registry.emplace<Nova::Components::TagComponent>(entity, tag);
            }
            return entity;
        }

        entt::entity GetViewportCamera() const { return m_ViewportCamera; }

        entt::entity CreateViewportCamera(const std::string& tag = "ViewportCamera") {
            entt::entity cam = CreateEntity(tag);
            m_Registry.emplace<Nova::Components::CameraComponent>(cam);
            auto &camera = Registry().get<Nova::Components::CameraComponent>(cam);
            camera.m_IsViewportCamera= true;
            m_ViewportCamera = cam;
            return cam;
        }

        template<typename... Comps, typename Func>
        void ForEach(Func&& system) {
            m_Registry.view<Comps...>().each(std::forward<Func>(system));
        }

        void AddToSelection(entt::entity e)      {m_Selected.insert(e);}
        void RemoveFromSelection(entt::entity e) { m_Selected.erase(e); }
        void ClearSelection()                    { m_Selected.clear(); }
        bool IsSelected(entt::entity e) const    { return m_Selected.count(e) > 0; }
        bool HasSelection() const               { return !m_Selected.empty(); }
        void DestroyEntity(entt::entity e) {
            if (IsSelected(e)) RemoveFromSelection(e);
            if (m_Registry.valid(e)) m_Registry.destroy(e);
        }
        const std::unordered_set<entt::entity>& GetSelected() const { return m_Selected; }

    private:

        entt::registry m_Registry;
        entt::entity m_ViewportCamera = entt::null;
        std::unordered_set<entt::entity> m_Selected;
    };
    
} // namespace Nova

#endif // SCENE_HPP