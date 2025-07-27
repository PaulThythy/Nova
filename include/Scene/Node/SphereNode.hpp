#ifndef SPHERE_NODE_HPP
#define SPHERE_NODE_HPP

#include <glm/glm.hpp>

#include "Scene/Node/Node.hpp"

namespace Nova {
    namespace Scene {

        struct SphereNode : public Node {
            float m_Radius = 1.0f;

            SphereNode(const std::string &name) : Node(name) {}
            ~SphereNode() = default;

        };

    } // namespace Scene
} // namespace Nova

#endif // SPHERE_NODE_HPP