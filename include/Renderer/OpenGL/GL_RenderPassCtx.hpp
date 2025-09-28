#ifndef GL_RENDER_PASS_CTX_HPP
#define GL_RENDER_PASS_CTX_HPP

#include <glm/glm.hpp>

namespace Nova { class Scene; }

namespace Nova::Renderer::OpenGL {
    struct GL_RenderPassCtx {
        //scene + viewport
        Nova::Scene* m_Scene = nullptr;
        unsigned     m_FBO   = 0;
        int          m_Width = 1;
        int          m_Height= 1;

        //camera
        glm::mat4    m_View{1.0f};
        glm::mat4    m_Projection{1.0f};
        float        m_Near = 0.1f;
        float        m_Far  = 1000.0f;
    };
}

#endif // GL_RENDER_PASS_CTX_HPP