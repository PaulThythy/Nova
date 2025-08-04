#ifndef NOVA_CORE_APPLICATION_HPP
#define NOVA_CORE_APPLICATION_HPP

#include <SDL3/SDL.h>

#include <GL/glew.h>

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL3/SDL_opengles2.h>
#else
#include <SDL3/SDL_opengl.h>
#endif

#include <memory>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

#include "Renderer/Renderer.hpp"
#include "Scene/Scene.hpp"

namespace Nova::Core {

    class Application {
    public:
        Application();
        ~Application();

        void run();

    private:
        bool m_IsRunning;

        bool m_ShowDemoWindow       = true;
        bool m_ShowAnotherWindow    = true;

        void initEngine();
        void destroyEngine();

        void setupSDL();
        void initSDL();
        void destroySDL();

        void initImGui();
        void destroyImGui();

        SDL_Window* m_Window        = nullptr;
        SDL_GLContext m_GLContext   = nullptr;

        std::unique_ptr<Nova::Renderer::IRenderer> m_Renderer = nullptr;
        Nova::Scene m_Scene;
    };
} // namespace Nova::Core

#endif // NOVA_CORE_APPLICATION_HPP