#ifndef GEOMETRY_PASS_HPP
#define GEOMETRY_PASS_HPP

#include <memory>
#include <unordered_map>

#include "Renderer/OpenGL/RenderPass.hpp"
#include "Renderer/OpenGL/GLMeshBuffers.hpp"

namespace Nova::Renderer::OpenGL {

    class GeometryPass : public IRenderPass {
    public:
        explicit GeometryPass(std::unordered_map<entt::entity, GLMeshBuffers>* cache);
        void execute(const RenderContext& ctx) override;
    private:
        std::unordered_map<entt::entity, GLMeshBuffers>* m_Cache;
        unsigned m_Program=0; // lit PBR-ish
    };

}

#endif // GEOMETRY_PASS_HPP