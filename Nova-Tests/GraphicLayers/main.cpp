#include <iostream>
#include "Core/Log.h"
#include "Core/Window.h"
#include "Core/Application.h"

#include "EmptyLayer.h"

int main(int /*argc*/, char** /*argv*/)
{
    NV_LOG_INFO("Starting Nova Graphic Layers Test");

    Nova::Core::Window::WindowDesc windowDesc;
    windowDesc.m_Title = "Nova Graphic Layers Test";
    windowDesc.m_Width = 1500;
    windowDesc.m_Height = 900;
    windowDesc.m_Resizable = true;
    windowDesc.m_VSync = true;
    windowDesc.m_GraphicsAPI = Nova::Core::GraphicsAPI::Vulkan;

    NV_LOG_INFO("Creating Nova Graphic Layers Test Application");
    Nova::Core::Application windowedApp(windowDesc);
    windowedApp.GetLayerStack().PushLayer<EmptyLayer>();
    windowedApp.Run();
    NV_LOG_INFO("Deleting Nova Graphic Layers Test Application");
    return 0;
}