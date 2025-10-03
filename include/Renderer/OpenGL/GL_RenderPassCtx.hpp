#ifndef GL_RENDER_PASS_CTX_HPP
#define GL_RENDER_PASS_CTX_HPP

#include <glm/glm.hpp>
#include <vector>

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

        struct GPULight {
            alignas(16) glm::vec3 m_Position;
            alignas(16) glm::vec3 m_Direction;    // for spot
            alignas(16) glm::vec3 m_Color;
            alignas(4) float m_Intensity;
            alignas(4) int m_LightShadows;     // bool as intz
            alignas(4) int m_Type;             // LightType as int
            alignas(4) float m_Range;          // for spot/point
            alignas(4) float m_InnerCos;       // for spot
            alignas(4) float m_OuterCos;       // for spot
        };

        std::vector<GPULight> m_Lights;
        int m_NumberOfLights = 0;
    };
}

#endif // GL_RENDER_PASS_CTX_HPP