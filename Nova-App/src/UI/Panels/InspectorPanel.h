#ifndef INSPECTORPANEL_H
#define INSPECTORPANEL_H

#include "Scene/Scene.h"

namespace Nova::App::Editor {
    class EditorLayer;
}

namespace Nova::App::UI::Panels::InspectorPanel {

    void Render(Nova::Core::Scene::Scene& scene, Editor::EditorLayer* editor = nullptr);

} // namespace Nova::App::UI::Panels::InspectorPanel

#endif // INSPECTORPANEL_H