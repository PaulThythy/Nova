#ifndef NOVA_CORE_APPLICATION_HPP
#define NOVA_CORE_APPLICATION_HPP

namespace Nova {
    namespace Core {

        class Application {
        public:
            Application();
            ~Application();

            bool initialize();

            void run();

            void destroy();

        private:
            bool m_isRunning;
        };

    } // namespace Core
} // namespace Nova

#endif // NOVA_CORE_APPLICATION_HPP