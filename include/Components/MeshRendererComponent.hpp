#ifndef MESH_RENDERER_COMPONENT_HPP
#define MESH_RENDERER_COMPONENT_HPP

#include <vector>
#include <glm/glm.hpp>

namespace Nova::Components {

    struct MeshRendererComponent {
        glm::vec3 m_BaseColor           {0.7f, 0.7f, 0.7f};
        float     m_Roughness             {0.5f};
        float     m_Metallic              {0.0f};
        glm::vec3 m_EmissiveColor         {0.0f, 0.0f, 0.0f};
        float     m_EmissiveStrength      {0.0f};

        bool      m_Visible               {true};
        bool      m_Wireframe             {false};
        bool      m_CastShadows           {true};

        MeshRendererComponent() = default;
    };

} // namespace Nova::Components

#endif // MESH_RENDERER_COMPONENT_HPP