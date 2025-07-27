#ifndef LIGHT_NODE_HPP
#define LIGHT_NODE_HPP

#include <glm/glm.hpp>

#include "Scene/Node/Node.hpp"

namespace Nova {
    namespace Scene {

        struct LightNode : public Node {
            glm::vec3 m_Color = {1.0f, 1.0f, 1.0f};
            float m_Intensity = 1.0f;

            LightNode(const std::string &name) : Node(name) {}
            ~LightNode() = default;

        };

    } // namespace Scene
} // namespace Nova

#endif // LIGHT_NODE_HPP