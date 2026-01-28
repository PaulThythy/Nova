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

	void VkLayer::OnBegin() {
		if (!m_Renderer) return;
		m_Renderer->BeginFrame();
	}
	
	void VkLayer::OnRender() {
		if (!m_Renderer) return;
		m_Renderer->Render();
	}

	void VkLayer::OnEnd() {
		if (!m_Renderer) return;
		m_Renderer->EndFrame();
	}

	void VkLayer::OnEvent(Event& e) {}

	void VkLayer::OnImGuiRender() {
		bool show_demo_window = true;
		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);
	}

} // namespace Nova::App