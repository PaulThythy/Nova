#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "Scene/Scene.hpp"

#include <memory>
#include <cstdint>

namespace Nova::Renderer {

    enum class GraphicsAPI {
        OpenGL
    };

    class IRenderer {
    public:
        virtual void init(Nova::Scene& scene) = 0;
        virtual void render() = 0;
        virtual void destroy() = 0;
        virtual void updateViewportSize(int width, int height) = 0;
        virtual void* getImGuiTextureID() const = 0;
        virtual ~IRenderer() = default;
    };

    std::unique_ptr<IRenderer> createRenderer(GraphicsAPI api);

} // namespace Nova::Renderer

#endif // RENDERER_HPP