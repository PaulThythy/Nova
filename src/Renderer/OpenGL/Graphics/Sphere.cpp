#include <glm/gtc/constants.hpp>

#include "Renderer/OpenGL/Graphics/Sphere.hpp"

namespace Nova {
    namespace Renderer {
        namespace OpenGL {
            namespace Graphics {

                void Sphere::init(int latitude, int longitude) {
                    std::vector<float> vertices;
                    std::vector<unsigned int> indices;
                    float radius = m_Node->m_Radius;
                    for(int lat = 0; lat <= latitude; ++lat) {
                        float theta = lat * glm::pi<float>() / latitude;
                        float sinTheta = sin(theta);
                        float cosTheta = cos(theta);
                        for(int lon = 0; lon <= longitude; ++lon) {
                            float phi = lon * 2.0f * glm::pi<float>() / longitude;
                            float sinPhi = sin(phi);
                            float cosPhi = cos(phi);

                            glm::vec3 pos{
                                cosPhi * sinTheta * radius,
                                cosTheta * radius,
                                sinPhi * sinTheta * radius
                            };
                            glm::vec3 normal = glm::normalize(pos);
                            vertices.push_back(pos.x);
                            vertices.push_back(pos.y);
                            vertices.push_back(pos.z);
                            vertices.push_back(normal.x);
                            vertices.push_back(normal.y);
                            vertices.push_back(normal.z);
                        }
                    }
                    for(int lat = 0; lat < latitude; ++lat) {
                        for(int lon = 0; lon < longitude; ++lon) {
                            int first = (lat * (longitude+1)) + lon;
                            int second = first + longitude + 1;
                            indices.push_back(first);
                            indices.push_back(second);
                            indices.push_back(first+1);
                            indices.push_back(second);
                            indices.push_back(second+1);
                            indices.push_back(first+1);
                        }
                    }
                    m_IndexCount = static_cast<GLsizei>(indices.size());

                    glGenVertexArrays(1, &m_VAO);
                    glGenBuffers(1, &m_VBO);
                    glGenBuffers(1, &m_EBO);

                    glBindVertexArray(m_VAO);

                    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
                    glBufferData(GL_ARRAY_BUFFER,
                                vertices.size()*sizeof(float),
                                vertices.data(), GL_STATIC_DRAW);

                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
                    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                                indices.size()*sizeof(unsigned int),
                                indices.data(), GL_STATIC_DRAW);

                    glEnableVertexAttribArray(0);
                    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
                    glEnableVertexAttribArray(1);
                    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));

                    glBindVertexArray(0);
                }

                void Sphere::render() {
                    glBindVertexArray(m_VAO);
                    glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, 0);
                    glBindVertexArray(0);
                }

                void Sphere::destroy() {
                    if(m_EBO) { glDeleteBuffers(1, &m_EBO); m_EBO = 0; }
                    if(m_VBO) { glDeleteBuffers(1, &m_VBO); m_VBO = 0; }
                    if(m_VAO) { glDeleteVertexArrays(1, &m_VAO); m_VAO = 0; }
                }

            } // namespace Graphics
        } // namespace OpenGL
    } // namespace Renderer
} // namespace Nova