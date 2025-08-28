#ifndef GL_RENDERER_HPP
#define GL_RENDERER_HPP

#include <GL/glew.h>
#include <entt/entt.hpp>
#include <unordered_map>

#include "Renderer/Renderer.hpp"
#include "Renderer/OpenGL/GL_RenderPass.hpp"
#include "Renderer/OpenGL/GL_MeshBuffers.hpp"
#include "Renderer/OpenGL/Passes/GL_ShadowPass.hpp"
#include "Renderer/OpenGL/Passes/GL_GeometryPass.hpp"
#include "Renderer/OpenGL/Passes/GL_OutlinePass.hpp"

namespace Nova { class Scene; }

namespace Nova::Renderer::OpenGL {

    class GL_ShadowPass;
    class GL_GeometryPass;
    class GL_OutlinePass;

    class GL_Renderer : public IRenderer {
    public:
        void Init(Nova::Scene& scene) override;
        void Render() override;
        void Destroy() override;

        void UpdateViewportSize(int width, int height) override;

        void* GetImGuiTextureID() const override { return reinterpret_cast<void*>(m_ColorTexture); }

        unsigned int GetShadowMapTexture() const override { return m_ShadowDepth;}
        int GetShadowMapSize() const override {return m_ShadowSize;}
    private:
        // scene
        Nova::Scene* m_Scene = nullptr;

        // viewport target
        unsigned m_FBO=0, m_ColorTexture=0, m_DepthStencil=0;
        int      m_W=1, m_H=1;

        // shadow target
        unsigned m_ShadowFBO=0, m_ShadowDepth=0; // sampler2DShadow
        int      m_ShadowSize = 4096;

        // caches
        std::unordered_map<entt::entity, GL_MeshBuffers> m_MeshCache;

        // passes
        std::unique_ptr<GL_ShadowPass>   m_ShadowPass;
        std::unique_ptr<GL_GeometryPass> m_GeometryPass;
        std::unique_ptr<GL_OutlinePass>  m_OutlinePass;

        GL_MeshBuffers CreateGLMeshBuffers(const Nova::Components::MeshComponent& mesh);

        // utils
        void BuildViewportFBO(int w, int h);
        void BuildShadowFBO();
        void OnMeshConstructed(entt::registry&, entt::entity);
        void OnMeshDestroyed (entt::registry&, entt::entity);
    };
} // namespace Nova::Renderer::OpenGL

#endif // GL_RENDERER_HPP