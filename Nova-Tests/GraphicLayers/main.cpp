#include <iostream>
#include "Core/Log.h"
#include "Core/Window.h"
#include "Core/Application.h"

#include "EmptyLayer.h"

int main(int /*argc*/, char** /*argv*/)
{
    Nova::Core::Window::WindowDesc windowDesc;
    windowDesc.m_Title = "Nova Graphic Layers Test";
    windowDesc.m_Width = 1500;
    windowDesc.m_Height = 900;
    windowDesc.m_Resizable = true;
    windowDesc.m_VSync = true;

    Nova::Core::Application windowedApp(windowDesc);
    windowedApp.GetLayerStack().PushLayer<EmptyLayer>();
    windowedApp.Run();
    return 0;
}