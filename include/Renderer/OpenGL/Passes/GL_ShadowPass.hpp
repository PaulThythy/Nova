#ifndef SHADOW_PASS_HPP
#define SHADOW_PASS_HPP

#include <memory>
#include <unordered_map>

#include "Renderer/OpenGL/GL_RenderPass.hpp"
#include "Renderer/OpenGL/GL_MeshBuffers.hpp"

namespace Nova::Renderer::OpenGL {

    class GL_ShadowPass : public IGL_RenderPass {
    public:
        GL_ShadowPass(std::unordered_map<entt::entity, GL_MeshBuffers>* cache, unsigned* fbo, unsigned* depthTex, int* size);
        void execute(const RenderContext& ctx) override;
    private:
        std::unordered_map<entt::entity, GL_MeshBuffers>* m_Cache;
        unsigned* m_FBO; 
        unsigned* m_Tex; 
        int* m_Size;
        unsigned m_Program = 0; // simple depth-only
    };
}

#endif // SHADOW_PASS_HPP