#ifndef VIEWPORT_PANEL_HPP
#define VIEWPORT_PANEL_HPP

#include "Scene/Scene.hpp"
#include "imgui.h"

namespace Nova::Renderer {
    class IRenderer;
}

namespace Nova::GUI::ViewportPanel {
    void Render(Nova::Renderer::IRenderer* renderer, Nova::Scene& scene);
    void Picking(Nova::Scene& scene, ImVec2 size);
    void Controls(Nova::Scene& scene);
    void ClearSelection(Nova::Scene& scene);
    void DeleteSelection(Nova::Scene& scene);
}

#endif //VIEWPORT_PANEL_HPP