#ifndef OUTLINE_PASS_HPP
#define OUTLINE_PASS_HPP

#include <memory>
#include <unordered_map>

#include "Renderer/OpenGL/RenderPass.hpp"
#include "Renderer/OpenGL/GLMeshBuffers.hpp"

namespace Nova::Renderer::OpenGL {

    class OutlinePass : public IRenderPass {
    public:
        explicit OutlinePass(std::unordered_map<entt::entity, GLMeshBuffers>* cache);
        void execute(const RenderContext& ctx) override;

    private:
        std::unordered_map<entt::entity, GLMeshBuffers>* m_Cache;
        unsigned m_OutlineProgram = 0; // solid color, extruded
    };

}

#endif // OUTLINE_PASS_HPP