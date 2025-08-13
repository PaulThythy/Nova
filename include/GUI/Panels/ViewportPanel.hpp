#ifndef VIEWPORT_PANEL_HPP
#define VIEWPORT_PANEL_HPP

#include "Scene/Scene.hpp"
#include "imgui.h"

namespace Nova::Renderer {
    class IRenderer;
}

namespace Nova::GUI {
    void renderViewportPanel(Nova::Renderer::IRenderer* renderer, Nova::Scene& scene);
    void picking(Nova::Scene& scene, ImVec2 size);
    void controls(Nova::Scene& scene);
    void clearSelection(Nova::Scene& scene);
    void deleteSelection(Nova::Scene& scene);
}

#endif //VIEWPORT_PANEL_HPP