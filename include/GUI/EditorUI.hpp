#ifndef EDITOR_UI_HPP
#define EDITOR_UI_HPP

#include "imgui.h"
#include "imgui_internal.h"
#include "GUI/Panels/HierarchyPanel.hpp"
#include "GUI/Panels/InspectorPanel.hpp"
#include "GUI/Panels/AssetBrowserPanel.hpp"
#include "GUI/Panels/ViewportPanel.hpp"

#include "Scene/Scene.hpp"


namespace Nova::Renderer {
    class IRenderer;
}

namespace Nova::GUI {
    void render(Nova::Renderer::IRenderer* renderer, Nova::Scene& scene);

    void setupDockSpace(bool useIni, ImGuiID dockspace_id);

} // namespace Nova::GUI

#endif // EDITOR_UI_HPP