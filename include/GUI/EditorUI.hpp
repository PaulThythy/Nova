#ifndef NOVA_GUI_EDITORUI_HPP
#define NOVA_GUI_EDITORUI_HPP

#include "imgui.h"
#include "imgui_internal.h"
#include "Renderer/OpenGL/OpenGLRenderer.hpp"

#include <GL/glew.h>

namespace Nova::GUI {

    void render(Nova::Renderer::OpenGL::OpenGLRenderer& renderer);

    void setupDockSpace(bool useIni, ImGuiID dockspace_id);

    void renderHierarchyPanel();
    void renderViewportPanel(Nova::Renderer::OpenGL::OpenGLRenderer& renderer);
    void renderInspectorPanel();
    void renderAssetBrowserPanel();

} // namespace Nova::GUI

#endif // NOVA_GUI_EDITORUI_HPP