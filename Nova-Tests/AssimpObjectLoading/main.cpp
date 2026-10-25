#include "Core/Application.h"
#include "Core/Window.h"
#include "Core/EntryPoint.h"

#include "AssimpLayer.h"

Nova::Core::Application* Nova::Core::CreateApplication(ApplicationCommandLineArgs /*args*/)
{
    Nova::Core::Window::WindowDesc windowDesc;
    windowDesc.m_Title = "Nova Assimp Object Loading Test";
    windowDesc.m_Width = 1500;
    windowDesc.m_Height = 900;
    windowDesc.m_Resizable = true;
    windowDesc.m_VSync = true;

    auto* app = new Nova::Core::Application(windowDesc);
    app->GetLayerStack().PushLayer<AssimpLayer>();
    return app;
}