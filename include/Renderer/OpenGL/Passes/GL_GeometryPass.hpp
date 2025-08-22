#ifndef GEOMETRY_PASS_HPP
#define GEOMETRY_PASS_HPP

#include <memory>
#include <unordered_map>

#include "Renderer/OpenGL/GL_RenderPass.hpp"
#include "Renderer/OpenGL/GL_MeshBuffers.hpp"

namespace Nova::Renderer::OpenGL {

    class GL_GeometryPass : public IGL_RenderPass {
    public:
        explicit GL_GeometryPass(std::unordered_map<entt::entity, GL_MeshBuffers>* cache);
        void execute(const RenderContext& ctx) override;
    private:
        std::unordered_map<entt::entity, GL_MeshBuffers>* m_Cache;
        unsigned m_Program=0; // lit PBR-ish
    };

}

#endif // GEOMETRY_PASS_HPP