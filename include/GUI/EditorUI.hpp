#ifndef EDITOR_UI_HPP
#define EDITOR_UI_HPP

#include "imgui.h"
#include "imgui_internal.h"
#include "GUI/Panels/HierarchyPanel.hpp"
#include "GUI/Panels/InspectorPanel.hpp"
#include "GUI/Panels/AssetBrowserPanel.hpp"
#include "GUI/Panels/ViewportPanel.hpp"
#include "Renderer/OpenGL/OpenGLRenderer.hpp"

#include <GL/glew.h>

namespace Nova::GUI {
    //TODO rework to pass any kind of renderer
    void render(Nova::Renderer::OpenGL::OpenGLRenderer& renderer);

    void setupDockSpace(bool useIni, ImGuiID dockspace_id);

} // namespace Nova::GUI

#endif // EDITOR_UI_HPP