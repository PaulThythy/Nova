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
	
	void VkLayer::OnUpdate(float dt) {
		m_DeltaTime = dt;
	}

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
		// Performance stats panel
		ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Performance", nullptr, ImGuiWindowFlags_None)) {
			float frameTimeMs = m_DeltaTime * 1000.0f;
			float fps = m_DeltaTime > 0.0f ? 1.0f / m_DeltaTime : 0.0f;
			
			ImGui::Text("Frame Time: %.2f ms", frameTimeMs);
			ImGui::Text("FPS: %.1f", fps);
		}
		ImGui::End();

		// Demo window for reference
		bool show_demo_window = true;
		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);
	}

} // namespace Nova::App