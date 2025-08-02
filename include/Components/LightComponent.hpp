#ifndef LIGHT_COMPONENT_HPP
#define LIGHT_COMPONENT_HPP

#include <glm/glm.hpp>

namespace Nova::Components {

    struct LightComponent {
        glm::vec3 m_Color = {1.0f, 1.0f, 1.0f};
        float m_Intensity = 1.0f;
    };

} // namespace Nova::Components

#endif // LIGHT_COMPONENT_HPP