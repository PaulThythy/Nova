#ifndef MESH_COMPONENT_HPP
#define MESH_COMPONENT_HPP

#include <vector>
#include <glm/glm.hpp>

namespace Nova::Components {

    struct MeshComponent {
        std::vector<glm::vec3> m_Vertices;
        std::vector<glm::vec3> m_Normals;
        std::vector<unsigned int> m_Indices;

        MeshComponent() = default;

        void initPlane() {
            m_Vertices.clear();
            m_Normals.clear();
            m_Indices.clear();

            m_Vertices = {
                {-0.5f, 0.0f, -0.5f},
                { 0.5f, 0.0f, -0.5f},
                { 0.5f, 0.0f,  0.5f},
                {-0.5f, 0.0f,  0.5f}
            };
            m_Normals.resize(m_Vertices.size(), glm::vec3(0.0f, 1.0f, 0.0f));
            m_Indices = {
                0, 1, 2,
                2, 3, 0
            };
        }

        void initSphere(int latitudeSegments = 16, int longitudeSegments = 32) {
            m_Vertices.clear();
            m_Normals.clear();
            m_Indices.clear();

            float radius = 1.0f;
            for (int y = 0; y <= latitudeSegments; ++y) {
                for (int x = 0; x <= longitudeSegments; ++x) {
                    float xSeg = float(x) / float(longitudeSegments);
                    float ySeg = float(y) / float(latitudeSegments);
                    float xPos = radius * cos(xSeg * 2.0f * glm::pi<float>()) * sin(ySeg * glm::pi<float>());
                    float yPos = radius * cos(ySeg * glm::pi<float>());
                    float zPos = radius * sin(xSeg * 2.0f * glm::pi<float>()) * sin(ySeg * glm::pi<float>());

                    m_Vertices.emplace_back(xPos, yPos, zPos);
                    m_Normals.emplace_back(glm::normalize(glm::vec3(xPos, yPos, zPos)));
                }
            }

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

} // namespace Nova::Components

#endif // MESH_COMPONENT_HPP