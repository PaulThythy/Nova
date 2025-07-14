#include "Core/Application.hpp"

#include <iostream>

namespace Nova {

    namespace Core {

        Application::Application()
            : m_isRunning(false)
        {
        }

        Application::~Application()
        {
            if (m_isRunning) {
                destroy();
            }
        }

        bool Application::initialize()
        {
            std::cout << "Initialization of Nova engine..." << std::endl;

            m_isRunning = true;
            return true;
        }

        void Application::run()
        {
            std::cout << "Starting main loop..." << std::endl;

            while (m_isRunning) {
                //render
                m_isRunning = false;
            }

            std::cout << "End of main loop." << std::endl;
        }

        void Application::destroy()
        {
            std::cout << "Destruction of Nova engine..." << std::endl;
            m_isRunning = false;
        }

    } // namespace Core
} // namespace Nova
