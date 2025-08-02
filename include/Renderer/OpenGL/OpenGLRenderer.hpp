#ifndef OPENGL_RENDERER_HPP
#define OPENGL_RENDERER_HPP

#include <GL/glew.h>

#include "Renderer/Renderer.hpp"
#include "Scene/Scene.hpp"
#include "Components/MeshComponent.hpp"
#include "Components/CameraComponent.hpp"
#include "Components/TransformComponent.hpp"
#include "Components/LightComponent.hpp"

namespace Nova {
    namespace Renderer {
        namespace OpenGL {

            class OpenGLRenderer : public IRenderer {
            public:
                void init(Nova::Scene& scene) override;
                void render() override;
                void destroy() override;

                void updateViewportSize(int width, int height) override;

                GLuint getTextureId() const { return m_ColorTexture; }

                int m_ViewportWidth = 1280;
                int m_ViewportHeight = 720;

            private:
                GLuint m_VAO = 0;
                GLuint m_VBO = 0;
                GLuint m_shaderProgram = 0;
                GLuint m_FBO = 0;
                GLuint m_ColorTexture = 0;
                GLuint m_DepthBuffer = 0;
                Nova::Scene* m_Scene = nullptr;

                void initFBO(int width, int height);
                GLuint uploadMesh(const Nova::Components::MeshComponent& mesh);
            };
        
        } // namespace OpenGL
    } // namespace Renderer
} // namespace Nova

#endif // OPENGL_RENDERER_HPP