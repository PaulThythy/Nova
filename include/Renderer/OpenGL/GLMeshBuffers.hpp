#ifndef GL_MESH_BUFFERS_HPP
#define GL_MESH_BUFFERS_HPP

#include <cstdint>

namespace Nova::Renderer::OpenGL {
    struct GLMeshBuffers {
        unsigned m_VAO = 0;
        unsigned m_VBO = 0;
        unsigned m_IBO = 0;
    };
}

#endif // GL_MESH_BUFFERS_HPP