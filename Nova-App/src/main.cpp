#include "Core/Application.h"
#include "Core/Window.h"

#include "App/AppLayer.h"
#include "App/EditorLayer.h"

int main() {
    Nova::Core::Window::WindowDesc windowDesc;
    windowDesc.m_Title = "Nova Engine";
    windowDesc.m_Width = 1500;
    windowDesc.m_Height = 900;
    windowDesc.m_Resizable = true;
    windowDesc.m_VSync = true;

    Nova::Core::Application windowedApp(windowDesc);
    windowedApp.GetLayerStack().PushOverlay<Nova::App::AppLayer>();
    windowedApp.GetLayerStack().PushLayer<Nova::App::EditorLayer>();
    windowedApp.Run();
}