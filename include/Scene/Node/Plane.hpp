#ifndef PLANE_HPP
#define PLANE_HPP

#include <vector>
#include <glm/glm.hpp>

#include "Scene/Node/Node.hpp"

namespace Nova {
    namespace Scene {

        struct Plane : public Node {

            std::vector<glm::vec3> m_Vertices;
            std::vector<glm::vec3> m_Normals;
            std::vector<unsigned int> m_Indices;

            Plane(const std::string& name = "Plane") : Node(name) {}

            void init() {
                m_Vertices.clear();
                m_Indices.clear();

                m_Vertices = {
                    {-0.5f, 0.0f, -0.5f},
                    { 0.5f, 0.0f, -0.5f},
                    { 0.5f, 0.0f,  0.5f},
                    {-0.5f, 0.0f,  0.5f}
                };

                m_Normals = {
                    {0.0f, 1.0f, 0.0f},
                    {0.0f, 1.0f, 0.0f},
                    {0.0f, 1.0f, 0.0f},
                    {0.0f, 1.0f, 0.0f}
                };

                m_Indices = {
                    0, 1, 2,
                    2, 3, 0
                };
            }
        };

    } // namespace Scene
} // namespace Nova

#endif // PLANE_HPP