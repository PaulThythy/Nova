#ifndef GL_CACHES_HPP
#define GL_CACHES_HPP

#include <cstdint>
#include <entt/entt.hpp>
#include <unordered_map>

namespace Nova::Renderer::OpenGL {
    struct GL_MeshBuffers {
        unsigned m_VAO = 0;
        unsigned m_VBO = 0;
        unsigned m_IBO = 0;
    };

    struct GL_LightBuffers {
        unsigned m_BufferID = 0;
    };

    inline std::unordered_map<entt::entity, GL_MeshBuffers> m_MeshCache;
    inline std::unordered_map<entt::entity, GL_LightBuffers> m_LightCache;
}

#endif // GL_CACHES_HPP