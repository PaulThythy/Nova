#include "Renderer/Renderer.hpp"
#include "Renderer/OpenGL/OpenGLRenderer.hpp"

namespace Nova::Renderer {

    IRenderer* createRenderer(GraphicsAPI api) {
        switch(api) {
            case GraphicsAPI::OpenGL:
                return new OpenGL::OpenGLRenderer();
            default:
                return nullptr;
        }
    }

} // namespace Nova::Renderer