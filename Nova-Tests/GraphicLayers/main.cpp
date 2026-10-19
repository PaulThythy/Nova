#include "Core/Application.h"
#include "Core/Window.h"
#include "Core/EntryPoint.h"

#include "EmptyLayer.h"

Nova::Core::Application* Nova::Core::CreateApplication(ApplicationCommandLineArgs /*args*/)
{
    Nova::Core::Window::WindowDesc windowDesc;
    windowDesc.m_Title = "Nova Graphic Layers Test";
    windowDesc.m_Width = 1500;
    windowDesc.m_Height = 900;
    windowDesc.m_Resizable = true;
    windowDesc.m_VSync = true;

    auto* app = new Nova::Core::Application(windowDesc);
    app->GetLayerStack().PushLayer<EmptyLayer>();
    return app;
}