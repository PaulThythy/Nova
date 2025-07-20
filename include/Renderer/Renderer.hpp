namespace Nova {
    namespace Renderer {

        class IRenderer {
        public:
            virtual void init(int width, int height) = 0;
            virtual void render() = 0;
            virtual void destroy() = 0;
            virtual void onResize(int width, int height) = 0;
            virtual ~IRenderer() = default;
        };

    } // namespace Renderer
} // namespace Nova