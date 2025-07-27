#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>

#include "Scene/Node/CameraNode.hpp"

namespace Nova {
    namespace Renderer {
        namespace OpenGL {
            namespace Graphics {

                struct Camera {
                    Nova::Scene::CameraNode* m_Node;

                    GLuint m_UBO;

                    Camera(Nova::Scene::CameraNode* node) : m_Node(node) {}
                    ~Camera() = default;

                    void init();
                    void update();
                    void destroy();
                };

            } // namespace Graphics
        } // namespace OpenGL
    } // namespace Renderer
} // namespace Nova

#endif // CAMERA_HPP