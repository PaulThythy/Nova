#ifndef SCENE_HPP
#define SCENE_HPP

#include <entt/entt.hpp>
#include <functional>

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

        void destroyEntity(entt::entity entity) { 
            if (m_Registry.valid(entity)) {
                m_Registry.destroy(entity); 
            }
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

        template<typename Component>
        void onComponentCreate(std::function<void(entt::registry&, entt::entity)> callback) {
            m_Registry.on_construct<Component>().sink().connect(callback);
        }

        template<typename Component>
        void onComponentUpdate(std::function<void(entt::registry&, entt::entity)> callback) {
            m_Registry.on_update<Component>().sink().connect(callback);
        }

        template<typename Component>
        void onComponentDestroy(std::function<void(entt::registry&, entt::entity)> callback) {
            m_Registry.on_destroy<Component>().sink().connect(callback);
        }

        template<typename... Comps, typename Func>
        void forEach(Func&& system) {
            m_Registry.view<Comps...>().each(std::forward<Func>(system));
        }

        void setSelected(entt::entity e)         { m_Selected = e; }
        void clearSelected()                     { m_Selected = entt::null; }
        entt::entity getSelected() const         { return m_Selected; }
        bool hasSelection() const                { return m_Selected != entt::null; }

    private:

        entt::registry m_Registry;
        entt::entity m_ViewportCamera = entt::null;
        entt::entity m_Selected = entt::null;
    };
    
} // namespace Nova

#endif // SCENE_HPP