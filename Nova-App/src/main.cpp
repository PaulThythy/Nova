#include "Core/Application.h"
#include "Core/Window.h"

#include "Layer/AppLayer.h"

int main() {
    Nova::Core::Window::WindowDesc windowDesc;
    windowDesc.m_Title = "Nova Engine";
    windowDesc.m_Width = 720;
    windowDesc.m_Height = 500;
    windowDesc.m_Resizable = true;
    windowDesc.m_VSync = true;

    Nova::Core::Application windowedApp(windowDesc);
    windowedApp.PushLayer<Nova::App::AppLayer>();
    windowedApp.Run();
}