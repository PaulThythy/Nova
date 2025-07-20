#ifndef OPENGL_RENDERER_HPP
#define OPENGL_RENDERER_HPP

#include <GL/glew.h>

#include "Renderer/Renderer.hpp"

namespace Nova {
    namespace Renderer {

        namespace OpenGL {

            class OpenGLRenderer : public IRenderer {
            public:
                void init(int width, int height) override;
                void render() override;
                void destroy() override;

                void onResize(int width, int height) override;

                GLuint getTextureId() const { return m_ColorTexture; }

            private:
                GLuint m_VAO = 0;
                GLuint m_VBO = 0;
                GLuint m_shaderProgram = 0;
                GLuint m_FBO = 0;
                GLuint m_ColorTexture = 0;
                GLuint m_DepthBuffer = 0;
                int m_ViewportWidth = 1280;
                int m_ViewportHeight = 720;

                void initFBO(int width, int height);
            };
        }

    } // namespace Renderer
} // namespace Nova

#endif // OPENGL_RENDERER_HPP