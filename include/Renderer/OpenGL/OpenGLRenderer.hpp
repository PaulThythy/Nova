#ifndef OPENGL_RENDERER_HPP
#define OPENGL_RENDERER_HPP

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

    class OpenGLRenderer : public IRenderer {
    public:
        void init(Nova::Scene& scene) override;
        void render() override;
        void destroy() override;

        void updateViewportSize(int width, int height) override;

        void* getImGuiTextureID() const override { return reinterpret_cast<void*>(m_ColorTexture); }

        unsigned int getShadowMapTexture() const override { return m_ShadowDepth;}
        int getShadowMapSize() const override {return m_ShadowSize;}
    private:
        // scene
        Nova::Scene* m_Scene = nullptr;

        // viewport target
        unsigned m_FBO=0, m_ColorTexture=0, m_DepthStencil=0;
        int      m_W=1, m_H=1;

        // shadow target
        unsigned m_ShadowFBO=0, m_ShadowDepth=0; // sampler2DShadow
        int      m_ShadowSize = 2048;

        // caches
        std::unordered_map<entt::entity, GL_MeshBuffers> m_MeshCache;

        // passes
        std::unique_ptr<GL_ShadowPass>   m_ShadowPass;
        std::unique_ptr<GL_GeometryPass> m_GeometryPass;
        std::unique_ptr<GL_OutlinePass>  m_OutlinePass;

        GL_MeshBuffers createGLMeshBuffers(const Nova::Components::MeshComponent& mesh);

        // utils
        void buildViewportFBO(int w, int h);
        void buildShadowFBO();
        void onMeshConstructed(entt::registry&, entt::entity);
        void onMeshDestroyed (entt::registry&, entt::entity);
    };
} // namespace Nova::Renderer::OpenGL

#endif // OPENGL_RENDERER_HPP