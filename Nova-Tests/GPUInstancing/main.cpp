#include "Core/Application.h"
#include "Core/Window.h"

#include "GPUInstancingLayer.h"

int main(int /*argc*/, char** /*argv*/)
{
    Nova::Core::Window::WindowDesc windowDesc;
    windowDesc.m_Title = "Nova GPU Instancing Test";
    windowDesc.m_Width = 1600;
    windowDesc.m_Height = 900;
    windowDesc.m_Resizable = true;
    windowDesc.m_VSync = false;

    Nova::Core::Application app(windowDesc);
    app.GetLayerStack().PushLayer<GPUInstancingLayer>();
    app.Run();
    return 0;
}