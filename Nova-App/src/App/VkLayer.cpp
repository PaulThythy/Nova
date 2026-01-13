#include "App/VkLayer.h"

namespace Nova::App {

	VkLayer::~VkLayer() = default;

	void VkLayer::OnAttach() {
		m_Renderer = Nova::Core::Renderer::RHI::IRenderer::Create(GraphicsAPI::Vulkan);
	}

	void VkLayer::OnDetach() {
		if (m_Renderer) {
			m_Renderer->Destroy();
			m_Renderer.reset();
		}
	}
	
	void VkLayer::OnUpdate(float dt) {}
	
	void VkLayer::OnRender() {}

	void VkLayer::OnEvent(Event& e) {}

	void VkLayer::OnImGuiRender() {}

} // namespace Nova::App