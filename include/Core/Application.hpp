#ifndef NOVA_CORE_APPLICATION_HPP
#define NOVA_CORE_APPLICATION_HPP

#include <SDL3/SDL.h>

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL3/SDL_opengles2.h>
#else
#include <SDL3/SDL_opengl.h>
#endif

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

namespace Nova {
    namespace Core {

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

        };

    } // namespace Core
} // namespace Nova

#endif // NOVA_CORE_APPLICATION_HPP