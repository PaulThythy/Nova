#ifndef GL_RENDERER_HPP
#define GL_RENDERER_HPP

#include <GL/glew.h>
#include <entt/entt.hpp>
#include <unordered_map>

#include "Renderer/Renderer.hpp"
#include "Renderer/OpenGL/GL_RenderPass.hpp"
#include "Renderer/OpenGL/GL_RenderPassCtx.hpp"
#include "Renderer/OpenGL/GL_Caches.hpp"

namespace Nova { class Scene; }

namespace Nova::Renderer::OpenGL {

    struct GL_DepthPrePass;
    struct GL_GBufferPass;

    class GL_Renderer : public IRenderer {
    public:
        void Init(Nova::Scene& scene) override;
        void Render() override;
        void Destroy() override;

        void UpdateViewportSize(int width, int height) override;

        void* GetImGuiTextureID() const override { return reinterpret_cast<void*>(m_GPosition); }
    private:
        // scene
        Nova::Scene* m_Scene = nullptr;

        // viewport target
        unsigned m_FBO = 0;
        unsigned m_GPosition = 0;         // COLOR_ATTACHMENT0
        unsigned m_GNormal = 0;           // COLOR_ATTACHMENT1
        unsigned m_GAlbedoRoughness = 0;  // COLOR_ATTACHMENT2
        unsigned m_GMetallic = 0;         // COLOR_ATTACHMENT3
        unsigned m_DepthTex = 0;
        int      m_W = 1, m_H = 1;

        //passes 
        GL_DepthPrePass* m_DepthPrePass = nullptr;
        GL_GBufferPass*  m_GBufferPass  = nullptr;

        // utils
        void BuildViewportFBO(int w, int h);
        
        void OnMeshCreated(entt::registry&, entt::entity);
        void OnMeshDestroyed (entt::registry&, entt::entity);

        void OnLightCreated(entt::registry&, entt::entity);
        void OnLightDestroyed(entt::registry&, entt::entity);
    };
} // namespace Nova::Renderer::OpenGL

#endif // GL_RENDERER_HPP