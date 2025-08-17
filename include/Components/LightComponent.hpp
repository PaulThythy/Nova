#ifndef LIGHT_COMPONENT_HPP
#define LIGHT_COMPONENT_HPP

#include <glm/glm.hpp>

namespace Nova::Components {

    enum class LightType : uint32_t {
        Directional = 0,
        Spot = 1,
        Point = 2
    };

    struct LightComponent {
        glm::vec3 m_Color = {1.0f, 1.0f, 1.0f};
        float m_Intensity = 1.0f;
        bool      m_LightShadows{ false };
        LightType m_Type{ LightType::Directional };

        // Specific, ignored if not relevant
        float     m_Range{ 10.0f };   // Spot / Point
        float     m_InnerCone{ 15.0f };   // Spot, degre
        float     m_OuterCone{ 25.0f };   // Spot, degre

        // Helpers
        float innerCos() const { return std::cos(glm::radians(m_InnerCone)); }
        float outerCos() const { return std::cos(glm::radians(m_OuterCone)); }
    };

} // namespace Nova::Components

#endif // LIGHT_COMPONENT_HPP