#ifndef VIEWPORT_PANEL_HPP
#define VIEWPORT_PANEL_HPP

//TODO rework to pass any kind of renderer
namespace Nova::Renderer::OpenGL { class OpenGLRenderer; }

namespace Nova::GUI {
    void renderViewportPanel(Nova::Renderer::OpenGL::OpenGLRenderer& renderer);
}

#endif //VIEWPORT_PANEL_HPP