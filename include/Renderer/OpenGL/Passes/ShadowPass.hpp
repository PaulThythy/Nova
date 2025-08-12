#ifndef SHADOW_PASS_HPP
#define SHADOW_PASS_HPP

#include <memory>
#include <unordered_map>

#include "Renderer/OpenGL/RenderPass.hpp"
#include "Renderer/OpenGL/GLMeshBuffers.hpp"

namespace Nova::Renderer::OpenGL {

    class ShadowPass : public IRenderPass {
    public:
        ShadowPass(std::unordered_map<entt::entity, GLMeshBuffers>* cache, unsigned* fbo, unsigned* depthTex, int* size);
        void execute(const RenderContext& ctx) override;
    private:
        std::unordered_map<entt::entity, GLMeshBuffers>* m_Cache;
        unsigned* m_FBO; 
        unsigned* m_Tex; 
        int* m_Size;
        unsigned m_Program = 0; // simple depth-only
    };
}

#endif // SHADOW_PASS_HPP