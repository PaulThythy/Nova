#include "Core/Application.h"
#include "Core/Window.h"

#include "App/AppLayer.h"
#include "App/EditorLayer.h"
#include "App/VkLayer.h"

#include "Core/Log.h"

int main() {

    NV_LOG_INFO("Starting Nova Engine");

    Nova::Core::Window::WindowDesc windowDesc;
    windowDesc.m_Title = "Nova Engine";
    windowDesc.m_Width = 1500;
    windowDesc.m_Height = 900;
    windowDesc.m_Resizable = true;
    windowDesc.m_VSync = true;
    windowDesc.m_GraphicsAPI = GraphicsAPI::OpenGL;

    NV_LOG_INFO("Creating Nova Application");
    Nova::Core::Application windowedApp(windowDesc);
    windowedApp.GetLayerStack().PushOverlay<Nova::App::AppLayer>();
    windowedApp.GetLayerStack().PushLayer<Nova::App::EditorLayer>();
    //windowedApp.GetLayerStack().PushLayer<Nova::App::VkLayer>();
    windowedApp.Run();
    NV_LOG_INFO("Deleting Nova Application");
}