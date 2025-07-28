#ifndef SCENE_HPP
#define SCENE_HPP

#include <vector>
#include <algorithm>

#include "Scene/Node/Node.hpp"
#include "Scene/Node/Camera.hpp"

namespace Nova {
    namespace Scene {
        struct Scene {
            std::vector<Nova::Scene::Node*> m_Roots;

            Nova::Scene::Camera* m_ViewportCamera;

            Scene() = default;
            ~Scene();

            void addNode(Nova::Scene::Node* node);
            void removeRoot(Nova::Scene::Node* node);

            Nova::Scene::Node* findRootByName(const std::string& name) const;
        };

    } // namespace Scene
} // namespace Nova

#endif // SCENE_HPP