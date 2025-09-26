#ifndef NOVA_CORE_APPLICATION_HPP
#define NOVA_CORE_APPLICATION_HPP

#include <SDL3/SDL.h>

#include <GL/glew.h>

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL3/SDL_opengles2.h>
#else
#include <SDL3/SDL_opengl.h>
#endif

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

#include "Core/Window.hpp"
#include "Renderer/Renderer.hpp"
#include "Scene/Scene.hpp"
#include "Input/InputManager.hpp"

namespace Nova::Core {

    class Application {
    public:
        Application();
        ~Application();

        void Run();

        enum class RunMode {
            Editor,
            Game
        };
        void    SetRunMode(RunMode m) {
            m_RunMode = m;
            m_InputManager.SetRunMode(m);
        }
        RunMode runMode() const         { return m_RunMode; }

    private:
        bool m_IsRunning;

        bool m_ShowDemoWindow       = true;
        bool m_ShowAnotherWindow    = true;

        void InitEngine();
        void DestroyEngine();

        void InitWindow();
        void DestroyWindow();

        void InitImGui();
        void DestroyImGui();

        Window* m_Window = nullptr;

        Nova::Renderer::IRenderer* m_Renderer = nullptr;
        Nova::Scene m_Scene;

        RunMode m_RunMode;

        Nova::Input::InputManager& m_InputManager = Nova::Input::InputManager::Instance();
    };
} // namespace Nova::Core

#endif // NOVA_CORE_APPLICATION_HPP