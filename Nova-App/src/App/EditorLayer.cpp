#include "App/EditorLayer.h"

#include "App/AppLayer.h"
#include "Core/Assert.h"

namespace Nova::App {

    void EditorLayer::OnAttach() {
        if (g_AppLayer)
            g_AppLayer->RegisterEditorLayer(this);
    }

    void EditorLayer::OnDetach() {
        if (g_AppLayer)
            g_AppLayer->RegisterEditorLayer(nullptr);
    }

    void EditorLayer::OnUpdate(float) {}

    void EditorLayer::OnBegin() {}

    void EditorLayer::OnRender() {
        if (g_AppLayer)
            g_AppLayer->RenderScene();
    }

    void EditorLayer::OnEnd() {}

    void EditorLayer::OnImGuiRender() {}

    void EditorLayer::OnEvent(Nova::Core::Events::Event&) {}

} // namespace Nova::App