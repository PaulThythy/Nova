#ifndef MESH_COMPONENT_HPP
#define MESH_COMPONENT_HPP

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

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

        void initCube(float halfExtent = 0.5f) {
            m_Vertices.clear();
            m_Normals.clear();
            m_Indices.clear();

            const float h = halfExtent;

            auto addFace = [&](const glm::vec3& n,
                const glm::vec3& v0,
                const glm::vec3& v1,
                const glm::vec3& v2,
                const glm::vec3& v3)
                {
                    const uint32_t base = static_cast<uint32_t>(m_Vertices.size());
                    m_Vertices.push_back(v0); m_Normals.push_back(n);
                    m_Vertices.push_back(v1); m_Normals.push_back(n);
                    m_Vertices.push_back(v2); m_Normals.push_back(n);
                    m_Vertices.push_back(v3); m_Normals.push_back(n);

                    m_Indices.push_back(base + 0);
                    m_Indices.push_back(base + 1);
                    m_Indices.push_back(base + 2);
                    m_Indices.push_back(base + 2);
                    m_Indices.push_back(base + 3);
                    m_Indices.push_back(base + 0);
                };

            // +Z (front)
            addFace({ 0, 0, 1 },
                { -h, -h,  h }, { h, -h,  h }, { h,  h,  h }, { -h,  h,  h });
            // -Z (back)
            addFace({ 0, 0,-1 },
                { h, -h, -h }, { -h, -h, -h }, { -h,  h, -h }, { h,  h, -h });
            // -X (left)
            addFace({ -1, 0, 0 },
                { -h, -h, -h }, { -h, -h,  h }, { -h,  h,  h }, { -h,  h, -h });
            // +X (right)
            addFace({ 1, 0, 0 },
                { h, -h,  h }, { h, -h, -h }, { h,  h, -h }, { h,  h,  h });
            // +Y (top)
            addFace({ 0, 1, 0 },
                { -h,  h,  h }, { h,  h,  h }, { h,  h, -h }, { -h,  h, -h });
            // -Y (bottom)
            addFace({ 0,-1, 0 },
                { -h, -h, -h }, { h, -h, -h }, { h, -h,  h }, { -h, -h,  h });
        }

        void initCylinder(float radius = 0.5f, float height = 1.0f, int radialSegments = 32, int heightSegments = 1) {
            m_Vertices.clear();
            m_Normals.clear();
            m_Indices.clear();

            radialSegments = std::max(3, radialSegments);
            heightSegments = std::max(1, heightSegments);

            const float h = height * 0.5f;

            // -----------------------
            // (side)
            // -----------------------
            // We duplicate the joint vertex (i = radialSegments) to close it cleanly.
            for (int ySeg = 0; ySeg <= heightSegments; ++ySeg) {
                float v = float(ySeg) / float(heightSegments);
                float y = -h + v * height;

                for (int i = 0; i <= radialSegments; ++i) {
                    float u = float(i) / float(radialSegments);
                    float theta = u * glm::two_pi<float>();
                    float cx = std::cos(theta);
                    float sz = std::sin(theta);

                    glm::vec3 pos = { radius * cx, y, radius * sz };
                    glm::vec3 nrm = glm::normalize(glm::vec3(cx, 0.0f, sz)); // radial normals

                    m_Vertices.push_back(pos);
                    m_Normals.push_back(nrm);
                }
            }

            // Side indices (quads -> 2 triangles)
            const int ringStride = radialSegments + 1;
            for (int ySeg = 0; ySeg < heightSegments; ++ySeg) {
                int row0 = ySeg * ringStride;
                int row1 = (ySeg + 1) * ringStride;
                for (int i = 0; i < radialSegments; ++i) {
                    uint32_t i0 = row0 + i;
                    uint32_t i1 = row0 + i + 1;
                    uint32_t i2 = row1 + i;
                    uint32_t i3 = row1 + i + 1;

                    // CCW seen from the outside
                    m_Indices.push_back(i0); m_Indices.push_back(i2); m_Indices.push_back(i1);
                    m_Indices.push_back(i1); m_Indices.push_back(i2); m_Indices.push_back(i3);
                }
            }

            // -----------------------
            // (top)
            // -----------------------
            uint32_t baseTop = static_cast<uint32_t>(m_Vertices.size());
            // center
            m_Vertices.push_back({ 0.0f, h, 0.0f });
            m_Normals.push_back({ 0.0f, 1.0f, 0.0f });

            // ring
            for (int i = 0; i < radialSegments; ++i) {
                float u = float(i) / float(radialSegments);
                float theta = u * glm::two_pi<float>();
                float cx = std::cos(theta);
                float sz = std::sin(theta);
                m_Vertices.push_back({ radius * cx, h, radius * sz });
                m_Normals.push_back({ 0.0f, 1.0f, 0.0f });
            }

            // indices (CCW fan triangle seen from above)
            for (int i = 0; i < radialSegments; ++i) {
                uint32_t center = baseTop;
                uint32_t rim_i = baseTop + 1 + i;
                uint32_t rim_next = baseTop + 1 + ((i + 1) % radialSegments);
                m_Indices.push_back(center);
                m_Indices.push_back(rim_i);
                m_Indices.push_back(rim_next);
            }

            // -----------------------
            // (bottom)
            // -----------------------
            uint32_t baseBot = static_cast<uint32_t>(m_Vertices.size());
            // center
            m_Vertices.push_back({ 0.0f, -h, 0.0f });
            m_Normals.push_back({ 0.0f, -1.0f, 0.0f });

            // ring
            for (int i = 0; i < radialSegments; ++i) {
                float u = float(i) / float(radialSegments);
                float theta = u * glm::two_pi<float>();
                float cx = std::cos(theta);
                float sz = std::sin(theta);
                m_Vertices.push_back({ radius * cx, -h, radius * sz });
                m_Normals.push_back({ 0.0f, -1.0f, 0.0f });
            }

            // indices (CCW fan triangle seen from below -> reverse order here)
            for (int i = 0; i < radialSegments; ++i) {
                uint32_t center = baseBot;
                uint32_t rim_i = baseBot + 1 + i;
                uint32_t rim_next = baseBot + 1 + ((i + 1) % radialSegments);
                // To make the normal point towards -Y, we invert i/next.
                m_Indices.push_back(center);
                m_Indices.push_back(rim_next);
                m_Indices.push_back(rim_i);
            }
        }

        void initCapsule(float radius = 0.5f, float cylinderHeight = 1.0f, int radialSegments = 32, int heightSegments = 1, int hemisphereRings = 8) {
            m_Vertices.clear();
            m_Normals.clear();
            m_Indices.clear();

            // Safety clamps
            radialSegments = std::max(3, radialSegments);
            heightSegments = std::max(1, heightSegments);
            hemisphereRings = std::max(2, hemisphereRings);

            const float halfCyl = cylinderHeight * 0.5f;
            const int cols = radialSegments + 1; // duplicate last column to close the seam

            auto pushVertex = [&](const glm::vec3& p, const glm::vec3& n) -> uint32_t {
                m_Vertices.push_back(p);
                m_Normals.push_back(glm::normalize(n));
                return static_cast<uint32_t>(m_Vertices.size() - 1);
                };

            // ------------------------------------------------------------
            // 1) Cylinder side (from -halfCyl to +halfCyl)
            // ------------------------------------------------------------
            const uint32_t baseCyl = static_cast<uint32_t>(m_Vertices.size());
            for (int ySeg = 0; ySeg <= heightSegments; ++ySeg) {
                float v = float(ySeg) / float(heightSegments);
                float y = -halfCyl + v * cylinderHeight;

                for (int i = 0; i <= radialSegments; ++i) {
                    float u = float(i) / float(radialSegments);
                    float theta = u * glm::two_pi<float>();
                    float cx = std::cos(theta);
                    float sz = std::sin(theta);

                    glm::vec3 pos = { radius * cx, y, radius * sz };
                    glm::vec3 nrm = { cx, 0.0f, sz };  // radial normal
                    pushVertex(pos, nrm);
                }
            }

            // Cylinder indices (quads -> two triangles), CCW seen from outside
            for (int ySeg = 0; ySeg < heightSegments; ++ySeg) {
                uint32_t row0 = baseCyl + ySeg * cols;
                uint32_t row1 = baseCyl + (ySeg + 1) * cols;
                for (int i = 0; i < radialSegments; ++i) {
                    uint32_t i0 = row0 + i;
                    uint32_t i1 = row0 + i + 1;
                    uint32_t i2 = row1 + i;
                    uint32_t i3 = row1 + i + 1;

                    m_Indices.push_back(i0); m_Indices.push_back(i2); m_Indices.push_back(i1);
                    m_Indices.push_back(i1); m_Indices.push_back(i2); m_Indices.push_back(i3);
                }
            }

            // Helper to get the start of the top/bottom cylinder rings
            const uint32_t cylTopRingStart = baseCyl + heightSegments * cols; // y = +halfCyl
            const uint32_t cylBottomRingStart = baseCyl;                          // y = -halfCyl

            // ------------------------------------------------------------
            // 2) Top hemisphere (center at +halfCyl)
            //    Rings go from equator (phi=0) up to near pole (phi=pi/2).
            //    We reuse the cylinder's top ring as the first "previous ring".
            // ------------------------------------------------------------
            uint32_t prevRingStart = cylTopRingStart;
            for (int s = 1; s < hemisphereRings; ++s) {
                float t = float(s) / float(hemisphereRings);    // (0,1)
                float phi = t * (glm::pi<float>() * 0.5f);      // (0, pi/2)
                float yOff = std::sin(phi) * radius;
                float ringR = std::cos(phi) * radius;
                float y = halfCyl + yOff;

                uint32_t ringStart = static_cast<uint32_t>(m_Vertices.size());
                for (int i = 0; i <= radialSegments; ++i) {
                    float u = float(i) / float(radialSegments);
                    float theta = u * glm::two_pi<float>();
                    float cx = std::cos(theta);
                    float sz = std::sin(theta);

                    glm::vec3 pos = { ringR * cx, y, ringR * sz };
                    // normal from hemisphere center
                    glm::vec3 nrm = glm::normalize(glm::vec3(ringR * cx, yOff, ringR * sz));
                    pushVertex(pos, nrm);
                }

                // Connect previous ring (lower) to current ring (higher) with CCW
                for (int i = 0; i < radialSegments; ++i) {
                    uint32_t i0 = prevRingStart + i;       // lower
                    uint32_t i1 = prevRingStart + i + 1;
                    uint32_t i2 = ringStart + i;           // higher
                    uint32_t i3 = ringStart + i + 1;

                    m_Indices.push_back(i0); m_Indices.push_back(i2); m_Indices.push_back(i1);
                    m_Indices.push_back(i1); m_Indices.push_back(i2); m_Indices.push_back(i3);
                }

                prevRingStart = ringStart;
            }

            // Add top pole and connect with a fan (CCW seen from above)
            {
                uint32_t topPole = pushVertex({ 0.0f, halfCyl + radius, 0.0f }, { 0.0f, 1.0f, 0.0f });
                for (int i = 0; i < radialSegments; ++i) {
                    uint32_t rim_i = prevRingStart + i;
                    uint32_t rim_next = prevRingStart + i + 1;
                    m_Indices.push_back(topPole);
                    m_Indices.push_back(rim_i);
                    m_Indices.push_back(rim_next);
                }
            }

            // ------------------------------------------------------------
            // 3) Bottom hemisphere (center at -halfCyl)
            //    Rings go from equator (phi=0) down to near pole (phi=pi/2).
            //    We reuse the cylinder's bottom ring as the first "previous ring".
            // ------------------------------------------------------------
            prevRingStart = cylBottomRingStart;
            for (int s = 1; s < hemisphereRings; ++s) {
                float t = float(s) / float(hemisphereRings);    // (0,1)
                float phi = t * (glm::pi<float>() * 0.5f);      // (0, pi/2)
                float yOff = std::sin(phi) * radius;
                float ringR = std::cos(phi) * radius;
                float y = -halfCyl - yOff; // going downward

                uint32_t ringStart = static_cast<uint32_t>(m_Vertices.size());
                for (int i = 0; i <= radialSegments; ++i) {
                    float u = float(i) / float(radialSegments);
                    float theta = u * glm::two_pi<float>();
                    float cx = std::cos(theta);
                    float sz = std::sin(theta);

                    glm::vec3 pos = { ringR * cx, y, ringR * sz };
                    // normal from hemisphere center
                    glm::vec3 nrm = glm::normalize(glm::vec3(ringR * cx, -yOff, ringR * sz));
                    pushVertex(pos, nrm);
                }

                // For bottom, current ring is lower, previous is higher.
                // Keep CCW seen from outside by using (lower -> upper) pattern.
                for (int i = 0; i < radialSegments; ++i) {
                    uint32_t i0 = ringStart + i;           // lower
                    uint32_t i1 = ringStart + i + 1;
                    uint32_t i2 = prevRingStart + i;       // higher
                    uint32_t i3 = prevRingStart + i + 1;

                    m_Indices.push_back(i0); m_Indices.push_back(i2); m_Indices.push_back(i1);
                    m_Indices.push_back(i1); m_Indices.push_back(i2); m_Indices.push_back(i3);
                }

                prevRingStart = ringStart;
            }

            // Add bottom pole and connect with a fan (flip order to keep outward normals)
            {
                uint32_t botPole = pushVertex({ 0.0f, -halfCyl - radius, 0.0f }, { 0.0f, -1.0f, 0.0f });
                for (int i = 0; i < radialSegments; ++i) {
                    uint32_t rim_i = prevRingStart + i;
                    uint32_t rim_next = prevRingStart + i + 1;
                    // Reverse rim order for bottom so the winding stays CCW from the outside
                    m_Indices.push_back(botPole);
                    m_Indices.push_back(rim_next);
                    m_Indices.push_back(rim_i);
                }
            }
        }

    };

} // namespace Nova::Components

#endif // MESH_COMPONENT_HPP