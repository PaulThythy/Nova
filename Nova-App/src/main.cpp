#include "Core/Application.h"
#include "Core/Window.h"
#include "Core/EntryPoint.h"

#include "App/AppLayer.h"
#include "Editor/EditorLayer.h"

#include "Core/Log.h"

Nova::Core::Application* Nova::Core::CreateApplication(ApplicationCommandLineArgs /*args*/) {
    NV_LOG_INFO("Starting Nova Engine");

    Nova::Core::Window::WindowDesc windowDesc;
    windowDesc.m_Title = "Nova Engine";
    windowDesc.m_Width = 1500;
    windowDesc.m_Height = 900;
    windowDesc.m_Resizable = true;
    windowDesc.m_VSync = true;

    NV_LOG_INFO("Creating Nova Application");
    auto* app = new Nova::Core::Application(windowDesc);
    app->GetWindow().SetGraphicsAPI(Nova::Core::GraphicsAPI::Vulkan);
    app->GetLayerStack().PushOverlay<Nova::App::AppLayer>();
    app->GetLayerStack().PushLayer<Nova::App::Editor::EditorLayer>();
    return app;
}