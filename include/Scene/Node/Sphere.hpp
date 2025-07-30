#ifndef SPHERE_HPP
#define SPHERE_HPP

#include <vector>
#include <glm/glm.hpp>

#include "Scene/Node/Node.hpp"

namespace Nova {
    namespace Scene {

        struct Sphere : public Node {
            float m_Radius = 1.0f;

            std::vector<glm::vec3> m_Vertices;
            std::vector<glm::vec3> m_Normals;
            std::vector<unsigned int> m_Indices;

            Sphere(const std::string& name = "Sphere") : Node(name) {}

            void init(int latitudeSegments = 16, int longitudeSegments = 32) {
                m_Vertices.clear();
                m_Indices.clear();

                for (int y = 0; y <= latitudeSegments; ++y) {
                    for (int x = 0; x <= longitudeSegments; ++x) {
                        float xSegment = static_cast<float>(x) / longitudeSegments;
                        float ySegment = static_cast<float>(y) / latitudeSegments;
                        float xPos = m_Radius * std::cos(xSegment * 2.0f * glm::pi<float>()) * std::sin(ySegment * glm::pi<float>());
                        float yPos = m_Radius * std::cos(ySegment * glm::pi<float>());
                        float zPos = m_Radius * std::sin(xSegment * 2.0f * glm::pi<float>()) * std::sin(ySegment * glm::pi<float>());

                        m_Vertices.emplace_back(xPos, yPos, zPos);
                        m_Normals.emplace_back(glm::normalize(glm::vec3(xPos, yPos, zPos)));
                    }
                }

                bool oddRow = false;
                for (int y = 0; y < latitudeSegments; ++y) {
                    for (int x = 0; x < longitudeSegments; ++x) {
                        int i0 = y * (longitudeSegments + 1) + x;
                        int i1 = i0 + 1;
                        int i2 = i0 + longitudeSegments + 1;
                        int i3 = i2 + 1;

                        m_Indices.push_back(i0);
                        m_Indices.push_back(i2);
                        m_Indices.push_back(i1);

                        m_Indices.push_back(i1);
                        m_Indices.push_back(i2);
                        m_Indices.push_back(i3);
                    }
                }
            }
        };

    } // namespace Scene
} // namespace Nova

#endif // SPHERE_HPP