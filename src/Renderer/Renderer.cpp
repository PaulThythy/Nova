#include "Renderer/Renderer.hpp"
#include "Renderer/OpenGL/GL_Renderer.hpp"

namespace Nova::Renderer {

    IRenderer* CreateRenderer(GraphicsAPI api) {
        switch(api) {
            case GraphicsAPI::OpenGL:
                return new OpenGL::GL_Renderer();
            default:
                return nullptr;
        }
    }

} // namespace Nova::Renderer