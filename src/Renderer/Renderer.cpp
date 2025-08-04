#include "Renderer/Renderer.hpp"
#include "Renderer/OpenGL/OpenGLRenderer.hpp"

namespace Nova::Renderer {

    std::unique_ptr<IRenderer> createRenderer(GraphicsAPI api) {
        switch(api) {
            case GraphicsAPI::OpenGL:
                return std::make_unique<OpenGL::OpenGLRenderer>();
            default:
                return nullptr;
        }
    }

} // namespace Nova::Renderer