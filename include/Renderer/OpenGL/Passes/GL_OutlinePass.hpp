#ifndef OUTLINE_PASS_HPP
#define OUTLINE_PASS_HPP

#include <memory>
#include <unordered_map>

#include "Renderer/OpenGL/GL_RenderPass.hpp"
#include "Renderer/OpenGL/GL_MeshBuffers.hpp"

namespace Nova::Renderer::OpenGL {

    class GL_OutlinePass : public IGL_RenderPass {
    public:
        explicit GL_OutlinePass(std::unordered_map<entt::entity, GL_MeshBuffers>* cache);
        void execute(const RenderContext& ctx) override;

    private:
        std::unordered_map<entt::entity, GL_MeshBuffers>* m_Cache;
        unsigned m_OutlineProgram = 0; // solid color, extruded
    };

}

#endif // OUTLINE_PASS_HPP