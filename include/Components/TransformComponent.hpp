#ifndef TRANSFORM_COMPONENT_HPP
#define TRANSFORM_COMPONENT_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Nova::Components {

    struct TransformComponent {
        glm::vec3 m_Position = {0.0f, 0.0f, 0.0f};
        glm::vec3 m_Rotation = {0.0f, 0.0f, 0.0f};
        glm::vec3 m_Scale    = {1.0f, 1.0f, 1.0f};

        glm::mat4 GetTransform() const {
            glm::vec3 radians = glm::radians(m_Rotation);
            return glm::translate(glm::mat4(1.0f), m_Position)
                * glm::mat4_cast(glm::quat(radians))
                * glm::scale(glm::mat4(1.0f), m_Scale);
        }
    };

} // namespace Nova::Components

#endif // TRANSFORM_COMPONENT_HPP