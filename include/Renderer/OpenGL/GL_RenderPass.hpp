#ifndef RENDER_PASS_HPP
#define RENDER_PASS_HPP

#include <glm/glm.hpp>

#include "Scene/Scene.hpp"

namespace Nova::Renderer::OpenGL {

	struct RenderContext {
		// scene/camera
		Nova::Scene* m_Scene = nullptr;
		glm::mat4 m_View{ 1.0f };
		glm::mat4 m_Proj{ 1.0f };
		glm::vec3 m_CameraPos{0.0f};

		//lights
		bool m_HasLight = false;
		glm::vec3 m_LightPos{ 0.0f };
		glm::vec3 m_LightColor{ 1.0f };
		float m_LightIntensity = 1.0f;

		//shadows
		glm::mat4 m_LightVP{ 1.0f };
		unsigned m_ShadowTex = 0;
		int m_ShadowSize = 1024;
		float m_ShadowBias = 0.0008f;

		//targets
		unsigned m_FBO = 0;				//viewport FBO
		int m_ViewportWidth = 1;
		int m_ViewportHeight = 1;
	};

	struct IGL_RenderPass {
		virtual ~IGL_RenderPass() = default;
		virtual void execute(const RenderContext& ctx) = 0;
	};

} // namespace Nova::Renderer::OpenGL

#endif // RENDER_PASS_HPP

