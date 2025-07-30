#ifndef LIGHT_HPP
#define LIGHT_HPP

#include <glm/glm.hpp>

#include "Scene/Node/Node.hpp"

namespace Nova {
	namespace Scene {

		struct Light : public Node {
			glm::vec3 m_Color = { 1.0, 1.0, 1.0 };
			float m_Intensity = 1.0f;

			Light(const std::string& name = "Light"): Node(name) {}
		};

	} // namespace Scene
} // namespace Nova

#endif //LIGHT_APP