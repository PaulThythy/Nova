#ifndef VIEWPORT_PANEL_HPP
#define VIEWPORT_PANEL_HPP

#include "Scene/Scene.hpp"

namespace Nova::Renderer {
    class IRenderer;
}

namespace Nova::GUI {
    void renderViewportPanel(Nova::Renderer::IRenderer* renderer, Nova::Scene& scene);
}

#endif //VIEWPORT_PANEL_HPP