#ifndef CAMERA_NODE_HPP
#define CAMERA_NODE_HPP

#include <glm/glm.hpp>

#include "Scene/Node/Node.hpp"

namespace Nova {
    namespace Scene {

        struct CameraNode : public Node {
            glm::vec3 m_LookAt = {0.0f, 0.0f, 0.0f};
            glm::vec3 m_Up = {0.0f, 1.0f, 0.0f};
            float m_Fov = 45.0f;
            float m_NearPlane = 0.1f;
            float m_FarPlane = 100.0f;

            CameraNode(const std::string &name) : Node(name) {}
            ~CameraNode() = default;

        };

    } // namespace Scene
} // namespace Nova

#endif // CAMERA_NODE_HPP