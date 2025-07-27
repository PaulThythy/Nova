#ifndef SPHERE_HPP
#define SPHERE_HPP

#include <glm/glm.hpp>
#include <GL/glew.h>

#include "Scene/Node/SphereNode.hpp"

namespace Nova {
    namespace Renderer {
        namespace OpenGL {
            namespace Graphics {

                struct Sphere {

                    Nova::Scene::SphereNode* m_Node;

                    GLuint m_VAO;
                    GLuint m_VBO;
                    GLuint m_EBO;
                    GLsizei m_IndexCount;

                    Sphere(Nova::Scene::SphereNode* node) : m_Node(node) {}
                    ~Sphere() = default;

                    void init(int latitude, int longitude);
                    void render();
                    void destroy();
                };
            
            } // namespace Graphics
        } // namespace OpenGL
    } // namespace Renderer
} // namespace Nova

#endif // SPHERE_HPP