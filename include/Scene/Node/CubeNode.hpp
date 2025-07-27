#ifndef CUBE_NODE_HPP
#define CUBE_NODE_HPP

#include <glm/glm.hpp>

#include "Scene/Node/Node.hpp"

namespace Nova {
    namespace Scene {

        struct CubeNode : public Node {

            CubeNode(const std::string &name) : Node(name) {}
            ~CubeNode() = default;

        };

    } // namespace Scene
} // namespace Nova

#endif // CUBE_NODE_HPP