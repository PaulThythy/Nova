#include "Core/Application.hpp"
#include "GUI/EditorUI.hpp"

#include <Components/CameraComponent.hpp>
#include <Components/TransformComponent.hpp>
#include <Components/TagComponent.hpp>
#include <Components/MeshComponent.hpp>
#include <Components/LightComponent.hpp>

#include <iostream>
#include <filesystem>

namespace Nova {
    namespace Core {

        Application::Application(){
            m_IsRunning = false;
            initEngine();
        }

        void Application::initEngine() {
            setupSDL();
            initSDL();
            initImGui();

            auto cam = m_Scene.createViewportCamera("MainCamera");

            auto sphereEntity = m_Scene.createEntity("Sphere");
            m_Scene.registry().emplace<Nova::Components::TransformComponent>(sphereEntity);
            m_Scene.registry().emplace<Nova::Components::MeshComponent>(sphereEntity);
            auto &sphereMesh = m_Scene.registry().get<Nova::Components::MeshComponent>(sphereEntity);
            sphereMesh.initSphere(24, 48);

            auto lightEntity = m_Scene.createEntity("SunLight");
            m_Scene.registry().emplace<Nova::Components::TransformComponent>(lightEntity);
            m_Scene.registry().emplace<Nova::Components::LightComponent>(lightEntity);
            auto& light = m_Scene.registry().get<Nova::Components::LightComponent>(lightEntity);
            light.m_Intensity = 1.0f;
            auto& lightTransform = m_Scene.registry().get<Nova::Components::TransformComponent>(lightEntity);
            lightTransform.m_Position = {0.0f, 5.0f, 0.0f};

            /*auto* plane = new Nova::Scene::Plane("GroundPlane");
            plane->init();
            m_Scene.addNode(plane);*/

            m_Renderer.init(m_Scene);
        }

        void Application::setupSDL() {
            if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
                std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
                return;
            }
        }

        void Application::initSDL() {
            Uint32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
            m_Window = SDL_CreateWindow("Nova Engine", 1280, 720, windowFlags);
            if (!m_Window) {
                std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
                return;
            }

            m_GLContext = SDL_GL_CreateContext(m_Window);
            if (!m_GLContext) {
                std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << std::endl;
                return;
            }

            SDL_GL_MakeCurrent(m_Window, m_GLContext);
            SDL_GL_SetSwapInterval(1); // Enable vsync
            
            // Initialize OpenGL loader (e.g., glad, glew, etc.) here if needed

            SDL_ShowWindow(m_Window);
        }

        void Application::initImGui() {
            
            // Decide GL+GLSL versions
            #if defined(IMGUI_IMPL_OPENGL_ES2)
                // GL ES 2.0 + GLSL 100 (WebGL 1.0)
                const char* glslVersion = "#version 100";
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
            #elif defined(IMGUI_IMPL_OPENGL_ES3)
                // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
                const char* glslVersion = "#version 300 es";
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
            #elif defined(__APPLE__)
                // GL 3.2 Core + GLSL 150
                const char* glslVersion = "#version 150";
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
            #else
                // GL 3.0 + GLSL 130
                const char* glslVersion = "#version 130";
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
            #endif
                // other flags
                SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
                SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
                SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

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
            ImGui_ImplSDL3_InitForOpenGL(m_Window, m_GLContext);
            ImGui_ImplOpenGL3_Init(glslVersion); // Use the same GLSL
        }

        void Application::run() {
            ImVec4 clearColor(0.45f, 0.55f, 0.60f, 1.00f);
            m_IsRunning = true;
            while(m_IsRunning) {
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    ImGui_ImplSDL3_ProcessEvent(&event);
                    if (event.type == SDL_EVENT_QUIT) {
                        m_IsRunning = false;
                    }

                    if(event.type == SDL_EVENT_WINDOW_RESIZED) {
                        //int width = event.window.data1;
                        //int height = event.window.data2;
                        // resize event
                    }
                }

                if (SDL_GetWindowFlags(m_Window) & SDL_WINDOW_MINIMIZED) {
                    SDL_Delay(10);
                    continue;
                }

                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplSDL3_NewFrame();
                ImGui::NewFrame();

                // render UI

                Nova::GUI::render(m_Renderer);

                // Rendering
                ImGui::Render();
                glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
                glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
                glClear(GL_COLOR_BUFFER_BIT);

                m_Renderer.render();

                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

                // Update and Render additional Platform Windows
                if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                    SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
                    SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
                    ImGui::UpdatePlatformWindows();
                    ImGui::RenderPlatformWindowsDefault();
                    SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
                }

                SDL_GL_SwapWindow(m_Window); // Swap buffers
            }

            #ifdef __EMSCRIPTEN__
                EMSCRIPTEN_MAINLOOP_END;
            #endif
        }

        Application::~Application() {
            destroyEngine();
        }

        void Application::destroyEngine() {
            destroyImGui();
            destroySDL();
        }

        void Application::destroyImGui() {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext();
        }

        void Application::destroySDL() {
            SDL_GL_DestroyContext(m_GLContext);
            SDL_DestroyWindow(m_Window);
            SDL_Quit();
        }

    } // namespace Core
} // namespace Nova