#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "Scene/Scene.hpp"

namespace Nova::Renderer {

    enum class GraphicsAPI {
        OpenGL
    };

    class IRenderer {
    public:
        virtual void Init(Nova::Scene& scene) = 0;
        virtual void Render() = 0;
        virtual void Destroy() = 0;
        virtual void UpdateViewportSize(int width, int height) = 0;
        virtual void* GetImGuiTextureID() const = 0;
        virtual ~IRenderer() = default;
    };

    IRenderer* CreateRenderer(GraphicsAPI api);

} // namespace Nova::Renderer

#endif // RENDERER_HPP