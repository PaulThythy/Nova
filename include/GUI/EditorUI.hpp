#ifndef NOVA_GUI_EDITORUI_HPP
#define NOVA_GUI_EDITORUI_HPP

#include "imgui.h"
#include "imgui_internal.h"

#include <GL/glew.h>

namespace Nova {
    namespace GUI {

        void render(GLuint viewportTexture);

        void setupDockSpace(bool useIni, ImGuiID dockspace_id);

        void renderHierarchyPanel();
        void renderViewportPanel(GLuint viewportTexture);
        void renderInspectorPanel();
        void renderAssetBrowserPanel();

    } // namespace GUI
} // namespace Nova

#endif // NOVA_GUI_EDITORUI_HPP