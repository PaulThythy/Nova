#include "App/EditorLayer.h"

#include "App/AppLayer.h"
#include "Core/Assert.h"
#include "Core/Log.h"
#include "Core/Application.h"

#include "Renderer/RHI/RHI_ShaderCompiler.h"

#include <filesystem>

namespace Nova::App {

    void EditorLayer::CompileGridShaders() {
        namespace fs = std::filesystem;
        using namespace Nova::Core::Renderer::RHI;

        fs::path cwd            = fs::current_path();
        fs::path editorShaders  = cwd / "Nova-App" / "Resources" / "Editor" / "Shaders";
        fs::path engineShaders  = cwd / "Nova-Core" / "Resources" / "Engine" / "Shaders";

        RHI_ShaderCompileInput vertIn{};
        vertIn.m_File         = editorShaders / "Grid.vert.slang";
        vertIn.m_Stage        = RHI_ShaderStage::Vertex;
        vertIn.m_IncludeDirs.push_back(engineShaders);

        RHI_ShaderCompileInput fragIn{};
        fragIn.m_File         = editorShaders / "Grid.frag.slang";
        fragIn.m_Stage        = RHI_ShaderStage::Fragment;
        fragIn.m_IncludeDirs.push_back(engineShaders);

        auto* renderer = g_AppLayer ? g_AppLayer->GetRenderer() : nullptr;
        if (!renderer) {
            NV_LOG_ERROR("EditorLayer: renderer not available for grid shader creation");
            return;
        }

        m_GridShader = renderer->CreateFullscreenShader(vertIn, fragIn);
        if (m_GridShader)
            NV_LOG_INFO("Editor grid shader ready.");
        else
            NV_LOG_ERROR("Editor grid shader creation failed.");
    }

    void EditorLayer::OnAttach() {
        if (g_AppLayer)
            g_AppLayer->RegisterEditorLayer(this);

        CompileGridShaders();
    }

    void EditorLayer::OnDetach() {
        if (m_GridShader && g_AppLayer && g_AppLayer->GetRenderer()) {
            g_AppLayer->GetRenderer()->DestroyFullscreenShader(m_GridShader);
            m_GridShader = nullptr;
        }

        if (g_AppLayer)
            g_AppLayer->RegisterEditorLayer(nullptr);
    }

    void EditorLayer::OnUpdate(float) {}

    void EditorLayer::OnBegin() {}

    void EditorLayer::OnRender() {
        if (!g_AppLayer) return;

        if (m_GridShader && g_AppLayer->GetRenderer())
            g_AppLayer->GetRenderer()->DrawFullscreen(m_GridShader);

        g_AppLayer->RenderScene();
    }

    void EditorLayer::OnEnd() {}

    void EditorLayer::OnImGuiRender() {}

    void EditorLayer::OnEvent(Nova::Core::Events::Event&) {}

} // namespace Nova::App
