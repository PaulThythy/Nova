#include "Core/Application.hpp"
#include "GUI/EditorUI.hpp"

#include <Components/CameraComponent.hpp>
#include <Components/TransformComponent.hpp>
#include <Components/TagComponent.hpp>
#include <Components/MeshComponent.hpp>
#include <Components/LightComponent.hpp>

#include <iostream>
#include <filesystem>

namespace Nova::Core {

    Application::Application(){
        m_IsRunning = false;
        InitEngine();
    }

    void Application::InitWindow() {
        m_Window = new Window();
        Nova::Core::Window::WindowDesc desc;
        desc.m_Title = "Nova Engine";
        desc.m_Width = 1280;
        desc.m_Height = 720;
        desc.m_Resizable = true;
        desc.m_OpenGL = true;
        desc.m_GL_Major = 3;
        desc.m_GL_Minor = 3;
        desc.m_GL_Profile = SDL_GL_CONTEXT_PROFILE_CORE;
        desc.m_VSync = true;
        if (!m_Window->Create(desc)) {
            std::cerr << "Failed to create window\n";
            exit(EXIT_FAILURE);
        }
    }

    void Application::DestroyWindow() {
        if (m_Window) {
            m_Window->Destroy();
            delete m_Window;
            m_Window = nullptr;
        }
    }

    void Application::InitEngine() {
        InitWindow();
        InitImGui();
        SetRunMode(RunMode::Editor);

        m_Renderer = CreateRenderer(Nova::Renderer::GraphicsAPI::OpenGL);
        m_Renderer->Init(m_Scene);

        auto cam = m_Scene.CreateViewportCamera("MainCamera");

        auto sphereEntity = m_Scene.CreateEntity("Sphere");
        m_Scene.Registry().emplace<Nova::Components::TransformComponent>(sphereEntity);
        auto& sphereTf = m_Scene.Registry().get<Nova::Components::TransformComponent>(sphereEntity);
        sphereTf.m_Position = { 0.0f, 1.0f, 0.0f };

        Nova::Components::MeshComponent sphereMesh;
        sphereMesh.InitSphere(24, 48);
        m_Scene.Registry().emplace<Nova::Components::MeshComponent>(sphereEntity, std::move(sphereMesh));
        m_Scene.Registry().emplace<Nova::Components::MeshRendererComponent>(sphereEntity);

        auto planeEntity = m_Scene.CreateEntity("Ground");
        m_Scene.Registry().emplace<Nova::Components::TransformComponent>(planeEntity);
        auto& planeTf = m_Scene.Registry().get<Nova::Components::TransformComponent>(planeEntity);
        planeTf.m_Scale = { 10.0f, 10.0f, 10.0f };

        Nova::Components::MeshComponent planeMesh;
        planeMesh.InitPlane();
        m_Scene.Registry().emplace<Nova::Components::MeshComponent>(planeEntity, std::move(planeMesh));
        m_Scene.Registry().emplace<Nova::Components::MeshRendererComponent>(planeEntity);

        auto cubeEntity = m_Scene.CreateEntity("Cube");
        m_Scene.Registry().emplace<Nova::Components::TransformComponent>(cubeEntity);
        auto& cubeTf = m_Scene.Registry().get<Nova::Components::TransformComponent>(cubeEntity);
        cubeTf.m_Position = { -2.8f, 0.5f, 3.0f }; cubeTf.m_Rotation = { 0.0f, -0.7f, 0.0f };

        Nova::Components::MeshComponent cubeMesh;
        cubeMesh.InitCube();
        m_Scene.Registry().emplace<Nova::Components::MeshComponent>(cubeEntity, std::move(cubeMesh));
        m_Scene.Registry().emplace<Nova::Components::MeshRendererComponent>(cubeEntity);

        auto cylinderEntity = m_Scene.CreateEntity("Cylinder");
        m_Scene.Registry().emplace<Nova::Components::TransformComponent>(cylinderEntity);
        auto& cylinderTf = m_Scene.Registry().get<Nova::Components::TransformComponent>(cylinderEntity);
        cylinderTf.m_Position = { 3.0f, 0.5f, 1.5f };

        Nova::Components::MeshComponent cylinderMesh;
        cylinderMesh.InitCylinder();
        m_Scene.Registry().emplace<Nova::Components::MeshComponent>(cylinderEntity, std::move(cylinderMesh));
        m_Scene.Registry().emplace<Nova::Components::MeshRendererComponent>(cylinderEntity);

        auto capsuleEntity = m_Scene.CreateEntity("Capsule");
        m_Scene.Registry().emplace<Nova::Components::TransformComponent>(capsuleEntity);
        auto& capsuleTf = m_Scene.Registry().get<Nova::Components::TransformComponent>(capsuleEntity);
        capsuleTf.m_Position = { 1.0f, 0.5f, 2.2f }; capsuleTf.m_Rotation = { 0.0f, 0.0f, -90.0f };

        Nova::Components::MeshComponent capsuleMesh;
        capsuleMesh.InitCapsule();
        m_Scene.Registry().emplace<Nova::Components::MeshComponent>(capsuleEntity, std::move(capsuleMesh));
        m_Scene.Registry().emplace<Nova::Components::MeshRendererComponent>(capsuleEntity);

        // ---------- Directional ----------
        {
            auto e = m_Scene.CreateEntity("Sun");
            auto& tf = m_Scene.Registry().emplace<Nova::Components::TransformComponent>(e);
            tf.m_Rotation = { -45.0f, 45.0f, 0.0f };
            tf.m_Position = {0.0f, 4.0f, 0.0f};

            auto& li = m_Scene.Registry().emplace<Nova::Components::LightComponent>(e);
            li.m_Type = Nova::Components::LightType::Directional;
            li.m_Color = { 1.0f, 0.97f, 0.9f };
            li.m_Intensity = 1.0f;
            li.m_LightShadows = true;
        }
        

        // ---------- Point ----------
        /*{
            auto e = m_Scene.CreateEntity("Lamp");
            auto& tf = m_Scene.Registry().emplace<Nova::Components::TransformComponent>(e);
            tf.m_Position = { 2.0f, 2.0f, 2.0f };

            auto& li = m_Scene.Registry().emplace<Nova::Components::LightComponent>(e);
            li.m_Type = Nova::Components::LightType::Point;
            li.m_Color = { 1.0f, 0.7f, 0.5f };
            li.m_Intensity = 1.0f;
            li.m_Range = 7.0f;
            li.m_LightShadows = true;
        }*/

        // ---------- Spot ----------
        /*{
            auto e = m_Scene.CreateEntity("SpotLight");
            auto& tf = m_Scene.Registry().emplace<Nova::Components::TransformComponent>(e);
            tf.m_Position = { 0.0f, 3.0f, 0.0f };
            tf.m_Rotation = { -60.0f, 0.0f, 0.0f };

            auto& li = m_Scene.Registry().emplace<Nova::Components::LightComponent>(e);
            li.m_Type = Nova::Components::LightType::Spot;
            li.m_Color = { 1.0f, 1.0f, 1.0f };
            li.m_Intensity = 1.0f;
            li.m_Range = 12.0f;
            li.m_InnerCone = 15.0f;
            li.m_OuterCone = 25.0f;
            li.m_LightShadows = true;
        }*/
    }

    void Application::InitImGui() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();

        io.IniFilename = "imgui.ini";
        bool iniExisted = std::filesystem::exists(io.IniFilename);
        io.UserData = (void*)(intptr_t)iniExisted;

        // imgui flags
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;    // Enable Multi-Viewport / Platform Windows

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsLight();
        //ImGui::StyleColorsClassic();

        // Initialize backend SDL + OpenGL
        ImGui_ImplSDL3_InitForOpenGL(m_Window->GetSDLWindow(), m_Window->GetGLContext());
        ImGui_ImplOpenGL3_Init(m_Window->GetGLSLVersion()); // Use the same GLSL
    }

    void Application::Run() {
        ImVec4 clearColor(0.45f, 0.55f, 0.60f, 1.00f);
        m_IsRunning = true;
        while(m_IsRunning) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                ImGui_ImplSDL3_ProcessEvent(&event);
                //TODO to implement
                //m_InputManager.processEvent(event);
                if (event.type == SDL_EVENT_QUIT) {
                    m_IsRunning = false;
                }

                if(event.type == SDL_EVENT_WINDOW_RESIZED) {
                    //int width = event.window.data1;
                    //int height = event.window.data2;
                    // resize event
                }
            }

            if (SDL_GetWindowFlags(m_Window->GetSDLWindow()) & SDL_WINDOW_MINIMIZED) {
                SDL_Delay(10);
                continue;
            }

            //TODO to implement
            //m_InputManager.update();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();

            // render UI

            Nova::GUI::Render(m_Renderer, m_Scene);

            // Rendering
            ImGui::Render();
            glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
            glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
            glClear(GL_COLOR_BUFFER_BIT);

            m_Renderer->Render();

            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            // Update and Render additional Platform Windows
            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
                SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
                SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
            }

            SDL_GL_SwapWindow(m_Window->GetSDLWindow()); // Swap buffers
        }

        #ifdef __EMSCRIPTEN__
            EMSCRIPTEN_MAINLOOP_END;
        #endif
    }

    Application::~Application() {
        DestroyEngine();
    }

    void Application::DestroyEngine() {
        DestroyImGui();
        m_Window->Destroy();
    }

    void Application::DestroyImGui() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }
} // namespace Nova::Core