#ifndef INPUT_MANAGER_HPP
#define INPUT_MANAGER_HPP

#include <SDL3/SDL.h>
#include <SDL3/SDL_keycode.h>
#include <glm/glm.hpp>
#include <iostream>

#include "Input/InputAction.hpp"

//TODO complete rework !

namespace Nova::Input {

    class InputManager {
    public:
        static InputManager& instance() {
            static InputManager inst;
            return inst;
        }

        InputManager() {
            // Callbacks de debug (un simple cout chacun)
            m_ActionCallbacks[InputAction::CameraMoveForward]  = [] { std::cout << "[Input] CameraMoveForward"  << std::endl; };
            m_ActionCallbacks[InputAction::CameraMoveBackward] = [] { std::cout << "[Input] CameraMoveBackward" << std::endl; };
            m_ActionCallbacks[InputAction::CameraRotate]       = [] { std::cout << "[Input] CameraRotate"       << std::endl; };
            m_ActionCallbacks[InputAction::Camera2DTranslate]  = [] { std::cout << "[Input] Camera2DTranslate"  << std::endl; };
            m_ActionCallbacks[InputAction::CameraRotateLeft]   = [] { std::cout << "[Input] CameraRotateLeft"   << std::endl; };
            m_ActionCallbacks[InputAction::CameraRotateRight]  = [] { std::cout << "[Input] CameraRotateRight"  << std::endl; };
        }

        void dispatch(InputAction action) { trigger(action); }

        /** Update RunMode (0 = Editor, 1 = Game). */
        template<typename Enum>
        void setRunMode(Enum mode) { m_CurrentMode = static_cast<int>(mode); }

        void update() {
            m_MouseDelta = m_MousePosition - m_LastMousePosition;
            m_LastMousePosition = m_MousePosition;
            m_MouseWheelDelta = 0.0f;

            //EDITOR ONLY
            if (m_CurrentMode == 0) {
                const bool rmbDown   = isMouseButtonDown(SDL_BUTTON_RIGHT);
                const bool lmbDown   = isMouseButtonDown(SDL_BUTTON_LEFT);
                const bool shiftHeld = isKeyDown(SDLK_LSHIFT) || isKeyDown(SDLK_RSHIFT);

                if (rmbDown)                      trigger(InputAction::CameraRotate);
                if (lmbDown && shiftHeld)         trigger(InputAction::Camera2DTranslate);
            }

            std::cout << m_CurrentMode << std::endl;
        }

        void processEvent(const SDL_Event& event) {
            switch (event.type) {
                // --------------------- KEYBOARD ---------------------
                case SDL_EVENT_KEY_DOWN:
                    m_KeyStates[event.key.key] = true;
                    handleKeyEvent(event.key.key);
                    break;
                case SDL_EVENT_KEY_UP:
                    m_KeyStates[event.key.key] = false;
                    break;

                    // ---------------------- MOUSE -----------------------
                case SDL_EVENT_MOUSE_MOTION:
                    m_MousePosition = glm::vec2(event.motion.x, event.motion.y);
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    m_MouseButtonStates[event.button.button] = true;
                    handleMouseButton(event.button.button);
                    break;
                case SDL_EVENT_MOUSE_BUTTON_UP:
                    m_MouseButtonStates[event.button.button] = false;
                    break;
                case SDL_EVENT_MOUSE_WHEEL:
                    m_MouseWheelDelta = event.wheel.y;
                    handleMouseWheel();
                    break;
            }
        }

        // ------------------------------------------------------------
        //                       PUBLIC HELPERS
        // ------------------------------------------------------------
        void bindAction(InputAction action, const std::function<void()>& callback) {
            m_ActionCallbacks[action] = callback;
        }

        bool isKeyDown(SDL_Keycode code) const {
            auto it = m_KeyStates.find(code);
            return it != m_KeyStates.end() && it->second;
        }

        bool isMouseButtonDown(Uint8 button) const {
            auto it = m_MouseButtonStates.find(button);
            return it != m_MouseButtonStates.end() && it->second;
        }

        glm::vec2 mouseDelta() const { return m_MouseDelta; }
        float     wheelDelta() const { return m_MouseWheelDelta; }
    private:
        int m_CurrentMode = 0;                      // 0 = Editor, 1 = Game

        // ---------- Helpers ----------
        void trigger(InputAction action) {
            if (auto it = m_ActionCallbacks.find(action); it != m_ActionCallbacks.end()) {
                it->second();
            }
        }

        // ---------- Concrete handlers ----------
        void handleKeyEvent(SDL_Keycode code) {
            if (m_CurrentMode != 0 /*Editor*/) return;

            switch (code) {
                case SDLK_A:
                    trigger(InputAction::CameraRotateLeft);
                    break;
                case SDLK_E:
                    trigger(InputAction::CameraRotateRight);
                    break;
                default:
                    break;
            }
        }

        void handleMouseButton(Uint8 button) {
            if (m_CurrentMode != 0 /*Editor*/) return;
            const bool shiftHeld = isKeyDown(SDLK_LSHIFT) || isKeyDown(SDLK_RSHIFT);
            if (button == SDL_BUTTON_RIGHT) {
                trigger(InputAction::CameraRotate);
            } else if (button == SDL_BUTTON_LEFT && shiftHeld) {
                trigger(InputAction::Camera2DTranslate);
            }
        }

        void handleMouseWheel() {
            if (m_CurrentMode != 0 /*Editor*/) return;
            if (m_MouseWheelDelta > 0) trigger(InputAction::CameraMoveForward);
            else if (m_MouseWheelDelta < 0) trigger(InputAction::CameraMoveBackward);
        }

        std::unordered_map<SDL_Keycode, bool>   m_KeyStates;
        std::unordered_map<Uint8, bool>         m_MouseButtonStates;
        std::unordered_map<InputAction, std::function<void()>> m_ActionCallbacks;

        glm::vec2 m_MousePosition {0.0f};
        glm::vec2 m_LastMousePosition {0.0f};
        glm::vec2 m_MouseDelta {0.0f};
        float m_MouseWheelDelta = 0.0f;
    };
}

#endif//INPUT_MANAGER_HPP