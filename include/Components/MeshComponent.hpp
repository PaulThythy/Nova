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

                    m_Indices.push_back(base + 0); m_Indices.push_back(base + 1); m_Indices.push_back(base + 2);
                    m_Indices.push_back(base + 0); m_Indices.push_back(base + 2); m_Indices.push_back(base + 3);
                };

            // +Z (front) — outward normal (0,0,1), vertices laid CCW in space, but we emit **CW** via indices
            addFace({ 0, 0, 1 },
                { -h,  h,  h }, { h,  h,  h }, { h, -h,  h }, { -h, -h,  h });

            // -Z (back)
            addFace({ 0, 0,-1 },
                { h,  h, -h }, { -h,  h, -h }, { -h, -h, -h }, { h, -h, -h });

            // -X (left)
            addFace({ -1, 0, 0 },
                { -h,  h, -h }, { -h,  h,  h }, { -h, -h,  h }, { -h, -h, -h });

            // +X (right)
            addFace({ 1, 0, 0 },
                { h,  h,  h }, { h,  h, -h }, { h, -h, -h }, { h, -h,  h });

            // +Y (top)
            addFace({ 0, 1, 0 },
                { -h,  h, -h }, { h,  h, -h }, { h,  h,  h }, { -h,  h,  h });

            // -Y (bottom)
            addFace({ 0,-1, 0 },
                { -h, -h,  h }, { h, -h,  h }, { h, -h, -h }, { -h, -h, -h });
        }

        void initCylinder(float radius = 0.5f, float height = 1.0f, int radialSegments = 32, int heightSegments = 1) {
            m_Vertices.clear();
            m_Normals.clear();
            m_Indices.clear();

            radialSegments = std::max(3, radialSegments);
            heightSegments = std::max(1, heightSegments);

            const float h = height * 0.5f;

            // -----------------------------
            // Side (build rings bottom -> top)
            // -----------------------------
            for (int ySeg = 0; ySeg <= heightSegments; ++ySeg) {
                float v = float(ySeg) / float(heightSegments);
                float y = -h + v * height;

                for (int i = 0; i <= radialSegments; ++i) {
                    float u = float(i) / float(radialSegments);
                    float th = u * glm::two_pi<float>();
                    float cx = std::cos(th);
                    float sz = std::sin(th);

                    // Position on the ring
                    m_Vertices.emplace_back(radius * cx, y, radius * sz);
                    // Outward radial normal
                    m_Normals.emplace_back(glm::normalize(glm::vec3(cx, 0.0f, sz)));
                }
            }

            // Triangulate quads in **CW** seen from the outside.
            // Quad vertices layout (row0 = lower ring, row1 = upper ring):
            //   i0 = row0+i, i1 = row0+i+1, i2 = row1+i, i3 = row1+i+1
            // CW pattern: (i0, i1, i2) and (i1, i3, i2)
            const int stride = radialSegments + 1;
            for (int ySeg = 0; ySeg < heightSegments; ++ySeg) {
                int row0 = ySeg * stride;
                int row1 = (ySeg + 1) * stride;
                for (int i = 0; i < radialSegments; ++i) {
                    uint32_t i0 = row0 + i;
                    uint32_t i1 = row0 + i + 1;
                    uint32_t i2 = row1 + i;
                    uint32_t i3 = row1 + i + 1;

                    // CW
                    m_Indices.push_back(i0); m_Indices.push_back(i1); m_Indices.push_back(i2);
                    m_Indices.push_back(i1); m_Indices.push_back(i3); m_Indices.push_back(i2);
                }
            }

            // -----------------------------
            // Top cap (+Y)
            // -----------------------------
            // Fan is **CW when viewed from above** (outside normal = +Y).
            uint32_t baseTop = static_cast<uint32_t>(m_Vertices.size());
            // Center
            m_Vertices.emplace_back(0.0f, h, 0.0f);
            m_Normals.emplace_back(0.0f, 1.0f, 0.0f);

            // Rim (counter increases angle CCW, we will emit CW triangles)
            for (int i = 0; i < radialSegments; ++i) {
                float u = float(i) / float(radialSegments);
                float th = u * glm::two_pi<float>();
                m_Vertices.emplace_back(radius * std::cos(th), h, radius * std::sin(th));
                m_Normals.emplace_back(0.0f, 1.0f, 0.0f);
            }

            for (int i = 0; i < radialSegments; ++i) {
                uint32_t c = baseTop;                       // center
                uint32_t ri = baseTop + 1 + i;               // current rim
                uint32_t rn = baseTop + 1 + ((i + 1) % radialSegments); // next rim
                // CW: center -> current -> next (clockwise when looking from +Y)
                m_Indices.push_back(c); m_Indices.push_back(ri); m_Indices.push_back(rn);
            }

            // -----------------------------
            // Bottom cap (−Y)
            // -----------------------------
            // Fan is **CW when viewed from below** (outside normal = −Y).
            uint32_t baseBot = static_cast<uint32_t>(m_Vertices.size());
            // Center
            m_Vertices.emplace_back(0.0f, -h, 0.0f);
            m_Normals.emplace_back(0.0f, -1.0f, 0.0f);

            // Rim
            for (int i = 0; i < radialSegments; ++i) {
                float u = float(i) / float(radialSegments);
                float th = u * glm::two_pi<float>();
                m_Vertices.emplace_back(radius * std::cos(th), -h, radius * std::sin(th));
                m_Normals.emplace_back(0.0f, -1.0f, 0.0f);
            }

            for (int i = 0; i < radialSegments; ++i) {
                uint32_t c = baseBot;
                uint32_t ri = baseBot + 1 + i;
                uint32_t rn = baseBot + 1 + ((i + 1) % radialSegments);
                // When looking from the outside (−Y direction), CW means: center -> next -> current
                // But since the viewer is *below*, to keep "globally CW" with your GL state (glFrontFace(GL_CW)),
                // we emit in the same order as the top (c, ri, rn). This produces CW when seen from outside.
                m_Indices.push_back(c); m_Indices.push_back(rn); m_Indices.push_back(ri);
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
            const int   cols = radialSegments + 1; // duplicate last column to close the seam

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
                    float th = u * glm::two_pi<float>();
                    float cx = std::cos(th), sz = std::sin(th);

                    glm::vec3 pos = { radius * cx, y, radius * sz };
                    glm::vec3 nrm = { cx, 0.0f, sz }; // radial normal
                    pushVertex(pos, nrm);
                }
            }

            // Cylinder quads -> two triangles **CW** seen from the outside.
            // row0 = lower ring, row1 = upper ring
            // i0=row0+i, i1=row0+i+1, i2=row1+i, i3=row1+i+1
            // CW: (i0, i1, i2) and (i1, i3, i2)
            for (int ySeg = 0; ySeg < heightSegments; ++ySeg) {
                uint32_t row0 = baseCyl + ySeg * cols;
                uint32_t row1 = baseCyl + (ySeg + 1) * cols;
                for (int i = 0; i < radialSegments; ++i) {
                    uint32_t i0 = row0 + i;
                    uint32_t i1 = row0 + i + 1;
                    uint32_t i2 = row1 + i;
                    uint32_t i3 = row1 + i + 1;

                    m_Indices.push_back(i0); m_Indices.push_back(i1); m_Indices.push_back(i2);
                    m_Indices.push_back(i1); m_Indices.push_back(i3); m_Indices.push_back(i2);
                }
            }

            // Helpers to access the cylinder rings at the ends
            const uint32_t cylTopRingStart = baseCyl + heightSegments * cols; // y = +halfCyl (upper)
            const uint32_t cylBottomRingStart = baseCyl;                         // y = -halfCyl (lower)

            // ------------------------------------------------------------
            // 2) Top hemisphere (center at +halfCyl)
            //    Rings go from equator (phi=0) up to near the pole (phi=pi/2).
            //    Use the cylinder's top ring as the first "previous" ring.
            // ------------------------------------------------------------
            uint32_t prevRingStart = cylTopRingStart;
            for (int s = 1; s < hemisphereRings; ++s) {
                float t = float(s) / float(hemisphereRings); // (0,1)
                float phi = t * (glm::pi<float>() * 0.5f);     // (0, pi/2)
                float yOff = std::sin(phi) * radius;
                float ringR = std::cos(phi) * radius;
                float y = halfCyl + yOff;

                uint32_t ringStart = static_cast<uint32_t>(m_Vertices.size());
                for (int i = 0; i <= radialSegments; ++i) {
                    float u = float(i) / float(radialSegments);
                    float th = u * glm::two_pi<float>();
                    float cx = std::cos(th), sz = std::sin(th);

                    glm::vec3 pos = { ringR * cx, y, ringR * sz };
                    glm::vec3 nrm = glm::normalize(glm::vec3(ringR * cx, yOff, ringR * sz));
                    pushVertex(pos, nrm);
                }

                // Connect prev (lower) to curr (higher) with **CW** pattern
                for (int i = 0; i < radialSegments; ++i) {
                    uint32_t i0 = prevRingStart + i;       // lower
                    uint32_t i1 = prevRingStart + i + 1;
                    uint32_t i2 = ringStart + i;      // higher
                    uint32_t i3 = ringStart + i + 1;

                    m_Indices.push_back(i0); m_Indices.push_back(i1); m_Indices.push_back(i2);
                    m_Indices.push_back(i1); m_Indices.push_back(i3); m_Indices.push_back(i2);
                }

                prevRingStart = ringStart;
            }

            // Add top pole and connect with a **CW** fan (viewed from outside / +Y):
            {
                uint32_t topPole = pushVertex({ 0.0f, halfCyl + radius, 0.0f }, { 0.0f, 1.0f, 0.0f });
                for (int i = 0; i < radialSegments; ++i) {
                    uint32_t rim_i = prevRingStart + i;
                    uint32_t rim_next = prevRingStart + i + 1;
                    // CW: center -> current -> next
                    m_Indices.push_back(topPole);
                    m_Indices.push_back(rim_i);
                    m_Indices.push_back(rim_next);
                }
            }

            // ------------------------------------------------------------
            // 3) Bottom hemisphere (center at -halfCyl)
            //    Rings go from equator (phi=0) down to near the pole (phi=pi/2).
            //    Use the cylinder's bottom ring as the first "previous" ring.
            // ------------------------------------------------------------
            prevRingStart = cylBottomRingStart;
            for (int s = 1; s < hemisphereRings; ++s) {
                float t = float(s) / float(hemisphereRings); // (0,1)
                float phi = t * (glm::pi<float>() * 0.5f);     // (0, pi/2)
                float yOff = std::sin(phi) * radius;
                float ringR = std::cos(phi) * radius;
                float y = -halfCyl - yOff;                   // going downward

                uint32_t ringStart = static_cast<uint32_t>(m_Vertices.size());
                for (int i = 0; i <= radialSegments; ++i) {
                    float u = float(i) / float(radialSegments);
                    float th = u * glm::two_pi<float>();
                    float cx = std::cos(th), sz = std::sin(th);

                    glm::vec3 pos = { ringR * cx, y, ringR * sz };
                    glm::vec3 nrm = glm::normalize(glm::vec3(ringR * cx, -yOff, ringR * sz));
                    pushVertex(pos, nrm);
                }

                // prev = higher ring, curr = lower ring
                // Keep the same **CW** quad pattern (row0 = prev, row1 = curr):
                for (int i = 0; i < radialSegments; ++i) {
                    uint32_t i0 = prevRingStart + i;       // higher
                    uint32_t i1 = prevRingStart + i + 1;
                    uint32_t i2 = ringStart + i;      // lower
                    uint32_t i3 = ringStart + i + 1;

                    m_Indices.push_back(i0); m_Indices.push_back(i2); m_Indices.push_back(i1);
                    m_Indices.push_back(i1); m_Indices.push_back(i2); m_Indices.push_back(i3);
                }

                prevRingStart = ringStart;
            }

            // Add bottom pole and connect with a **CW** fan (viewed from outside / -Y):
            {
                uint32_t botPole = pushVertex({ 0.0f, -halfCyl - radius, 0.0f }, { 0.0f, -1.0f, 0.0f });
                for (int i = 0; i < radialSegments; ++i) {
                    uint32_t rim_i = prevRingStart + i;
                    uint32_t rim_next = prevRingStart + i + 1;

                    m_Indices.push_back(botPole);
                    m_Indices.push_back(rim_next);
                    m_Indices.push_back(rim_i);
                }
            }
        }

    };

} // namespace Nova::Components

#endif // MESH_COMPONENT_HPP