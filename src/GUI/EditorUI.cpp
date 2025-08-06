#include "GUI/EditorUI.hpp"

#include <cstdint>

namespace Nova::GUI {

    void setupDockSpace(bool useIni, ImGuiID dockspace_id) {
        static bool dock_initialized = false;
        if (!dock_initialized && !useIni && ImGui::DockBuilderGetNode(dockspace_id)) {
            dock_initialized = true;
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

            ImGuiID dock_down = 0, dock_main = 0;
            ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.19f, &dock_down, &dock_main);

            ImGuiID dock_right = 0, dock_center = 0; 
            ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.25f, &dock_right, &dock_center);

            ImGuiID dock_right_top = 0, dock_right_bottom = 0;
            ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Up, 0.5f, &dock_right_top, &dock_right_bottom);

            ImGui::DockBuilderDockWindow("Hierarchy", dock_right_top);
            ImGui::DockBuilderDockWindow("Inspector", dock_right_bottom);
            ImGui::DockBuilderDockWindow("Viewport",  dock_center);
            ImGui::DockBuilderDockWindow("Asset Browser", dock_down);

            ImGui::DockBuilderFinish(dockspace_id);
        }
    }

    void render(Nova::Renderer::IRenderer* renderer, Nova::Scene& scene) {
        bool useIni = (bool)(intptr_t)ImGui::GetIO().UserData;

        // ------ DockSpace ------
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGuiWindowFlags host_window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_UnsavedDocument
            | ImGuiWindowFlags_NoResize;

        ImGui::Begin("Nova Editor", nullptr, host_window_flags);
        ImGui::PopStyleVar(3);

        ImGuiID dockspace_id = ImGui::GetID("NovaDockspace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

        setupDockSpace(useIni, dockspace_id);

        ImGui::End();

        // ------ Panels ------
        renderHierarchyPanel(scene);
        renderViewportPanel(renderer, scene);
        renderInspectorPanel(scene);
        renderAssetBrowserPanel();
        renderMainMenuBar();
    }

} // namespace Nova::GUI