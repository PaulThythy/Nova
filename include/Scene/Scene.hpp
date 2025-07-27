#ifndef SCENE_HPP
#define SCENE_HPP

#include <vector>
#include <algorithm>

#include "Scene/Node/Node.hpp"

namespace Nova {
    namespace Scene {
        struct Scene {
            std::vector<Nova::Scene::Node*> m_Roots;

            Scene() = default;
            ~Scene();

            void addNode(Nova::Scene::Node* node);
            void removeRoot(Nova::Scene::Node* node);

            Nova::Scene::Node* findRootByName(const std::string& name) const;
        };

    } // namespace Scene
} // namespace Nova

#endif // SCENE_HPP