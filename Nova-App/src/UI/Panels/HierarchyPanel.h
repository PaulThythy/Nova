#ifndef HIERARCHYPANEL_H
#define HIERARCHYPANEL_H

#include "Scene/Scene.h"

namespace Nova::App::Editor {
    class EditorLayer;
}

namespace Nova::App::UI::Panels::HierarchyPanel {

    void Render(Nova::Core::Scene::Scene& scene, Editor::EditorLayer* editor = nullptr);

} // namespace Nova::App::UI::Panels::HierarchyPanel

#endif // HIERARCHYPANEL_H
