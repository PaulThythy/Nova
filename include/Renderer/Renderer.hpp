#include "Scene/Scene.hpp"

namespace Nova {
    namespace Renderer {

        class IRenderer {
        public:
            virtual void init(Nova::Scene& scene) = 0;
            virtual void render() = 0;
            virtual void destroy() = 0;
            virtual void updateViewportSize(int width, int height) = 0;
            virtual ~IRenderer() = default;
        };

    } // namespace Renderer
} // namespace Nova