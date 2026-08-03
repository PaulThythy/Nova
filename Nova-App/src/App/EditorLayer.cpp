#include "App/EditorLayer.h"

#include "App/AppLayer.h"
#include "Core/Assert.h"
#include "Core/Log.h"
#include "Core/Application.h"

#include "Renderer/RHI/RHI_ShaderCompiler.h"

#include <filesystem>

namespace Nova::App {

    void EditorLayer::OnAttach() {
        if (g_AppLayer) {
            g_AppLayer->RegisterEditorLayer(this);
            g_AppLayer->ShowGrid(true);
        }
    }

    void EditorLayer::OnDetach() {
        if (g_AppLayer) {
            g_AppLayer->RegisterEditorLayer(nullptr);
            g_AppLayer->ShowGrid(false);
        }
    }

    void EditorLayer::OnUpdate(float) {}

    void EditorLayer::OnBegin() {}

    void EditorLayer::OnRender() {}

    void EditorLayer::OnEnd() {}

    void EditorLayer::OnImGuiRender() {}

    void EditorLayer::OnEvent(Nova::Core::Events::Event&) {}

} // namespace Nova::App