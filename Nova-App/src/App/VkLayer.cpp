#include "App/VkLayer.h"

namespace Nova::App {

	VkLayer::~VkLayer() = default;

	void VkLayer::OnAttach() {
		GraphicsAPI api = Nova::Core::Application::Get().GetWindow().GetGraphicsAPI();
		m_Renderer = Nova::Core::Renderer::RHI::IRenderer::Create(api);

		// camera setup
		m_Camera = std::make_shared<Renderer::Graphics::Camera>(
            glm::vec3(2.5f, 2.5f, 5.0f),               // lookFrom
            glm::vec3(0.0f, 0.0f, 0.0f),                // lookAt
            glm::vec3(0.0f, 1.0f, 0.0f),                // up
            45.0f,                                      // FOV in degree
            16.0f / 9.0f,                               // aspect ratio
            0.1f,                                       // near
            100.0f,                                     // far
            true                                        // perspective
        );
		m_Camera->m_IsPerspective = true;
		m_Camera->m_FOV = 45.0f;
		m_Camera->m_NearPlane = 0.1f;
		m_Camera->m_FarPlane = 100.0f;
		m_Camera->m_Up = {0.0f, 1.0f, 0.0f};

        entt::entity cameraEntity = m_Scene.CreateEntity("Camera");

        m_Scene.SetMainCamera(cameraEntity);

		auto& registry = m_Scene.GetRegistry();
        registry.emplace<CameraComponent>(
            cameraEntity,
            m_Camera,
            true // isPrimary
        );

		// load asset
		auto cubeAsset = AssetManager::Get().Acquire<MeshAsset>("Engine://Primitives/Cube").GetAssetRef();
		cubeAsset->Load();
		entt::entity cubeEntity = m_Scene.CreateEntity("Cube");

		registry.emplace<TransformComponent>(cubeEntity,
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 1.0f)
        );
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

	void VkLayer::OnEvent(Event& e) {
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& ev) { return OnMouseButtonPressed(ev); });
		dispatcher.Dispatch<MouseButtonReleasedEvent>([this](MouseButtonReleasedEvent& ev) { return OnMouseButtonReleased(ev); });
		dispatcher.Dispatch<MouseMovedEvent>([this](MouseMovedEvent& ev) { return OnMouseMoved(ev); });
		dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent& ev) { return OnMouseScrolled(ev); });
		dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) { return OnWindowResized(ev); });
	}

	void VkLayer::UpdateCameraFromOrbit() {
		// Clamp pitch pour éviter les singularités (gimbal-ish / flip)
		const float maxPitch = glm::radians(89.0f);
		m_Orbit.m_Pitch = std::clamp(m_Orbit.m_Pitch, -maxPitch, maxPitch);
		m_Orbit.m_Distance = std::max(0.2f, m_Orbit.m_Distance);

		const float cp = std::cos(m_Orbit.m_Pitch);

		// yaw=0 => caméra sur +Z
		glm::vec3 offset;
		offset.x = m_Orbit.m_Distance * cp * std::sin(m_Orbit.m_Yaw);
		offset.y = m_Orbit.m_Distance * std::sin(m_Orbit.m_Pitch);
		offset.z = m_Orbit.m_Distance * cp * std::cos(m_Orbit.m_Yaw);

		m_Camera->m_LookAt = m_Orbit.m_Target;
		m_Camera->m_LookFrom = m_Orbit.m_Target + offset;
		m_Camera->m_Up = {0.0f, 1.0f, 0.0f};
	}

	void VkLayer::UpdateCameraAspectFromWindow() {
		SDL_Window* window = Nova::Core::Application::Get().GetWindow().GetSDLWindow();
		int w = 0, h = 0;
		SDL_GetWindowSizeInPixels(window, &w, &h);

		if (w > 0 && h > 0) {
			m_Camera->m_AspectRatio = static_cast<float>(w) / static_cast<float>(h);
		}
	}

	bool VkLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e) {
		// Adapte le nom du bouton à ton enum
		// Exemple: if (e.GetMouseButton() == MouseButton::Right)
		if (e.GetMouseButton() == 1 /* Right placeholder */) {
			m_Orbit.m_IsRotating = true;
			m_Orbit.m_HasLastMousePos = false; // reset pour éviter un gros delta au premier move
			return true;
		}
		return false;
	}

	bool VkLayer::OnMouseButtonReleased(MouseButtonReleasedEvent& e) {
		if (e.GetMouseButton() == 1 /* Right placeholder */) {
			m_Orbit.m_IsRotating = false;
			return true;
		}
		return false;
	}

	bool VkLayer::OnMouseMoved(MouseMovedEvent& e) {
		// Si ImGui capture la souris, ignore l'orbite (recommandé)
		// if (ImGui::GetIO().WantCaptureMouse) return false;

		const glm::vec2 mousePos{ e.GetX(), e.GetY() };

		if (!m_Orbit.m_IsRotating) {
			m_Orbit.m_LastMousePos = mousePos;
			m_Orbit.m_HasLastMousePos = true;
			return false;
		}

		if (!m_Orbit.m_HasLastMousePos) {
			m_Orbit.m_LastMousePos = mousePos;
			m_Orbit.m_HasLastMousePos = true;
			return true;
		}

		const glm::vec2 delta = mousePos - m_Orbit.m_LastMousePos;
		m_Orbit.m_LastMousePos = mousePos;

		// RMB drag: yaw/pitch
		m_Orbit.m_Yaw   -= delta.x * m_Orbit.m_RotateSensitivity;
		m_Orbit.m_Pitch -= delta.y * m_Orbit.m_RotateSensitivity;

		UpdateCameraFromOrbit();
		return true;
	}

	bool VkLayer::OnMouseScrolled(MouseScrolledEvent& e) {
		// Optionnel mais très utile : zoom orbit
		const float scrollY = e.GetYOffset();
		m_Orbit.m_Distance -= scrollY * m_Orbit.m_ZoomSensitivity;
		UpdateCameraFromOrbit();
		return true;
	}

	bool VkLayer::OnWindowResized(WindowResizeEvent& e) {
		if (e.GetWidth() <= 0 || e.GetHeight() <= 0) return false;
		m_Camera->m_AspectRatio = static_cast<float>(e.GetWidth()) / static_cast<float>(e.GetHeight());
		return false;
	}


	void VkLayer::OnImGuiRender() {
		// Performance stats panel
		ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Performance", nullptr, ImGuiWindowFlags_None)) {
			float frameTimeMs = m_DeltaTime * 1000.0f;
			float fps = m_DeltaTime > 0.0f ? 1.0f / m_DeltaTime : 0.0f;
			
			ImGui::Text("Frame Time: %.2f ms", frameTimeMs);
			ImGui::Text("FPS: %.1f", fps);

			ImGui::Separator();
			ImGui::Text("Orbit");
			ImGui::Text("Yaw: %.2f deg", glm::degrees(m_Orbit.m_Yaw));
			ImGui::Text("Pitch: %.2f deg", glm::degrees(m_Orbit.m_Pitch));
			ImGui::Text("Distance: %.2f", m_Orbit.m_Distance);
			ImGui::Text("Camera Pos: (%.2f, %.2f, %.2f)",
				m_Camera->m_LookFrom.x, m_Camera->m_LookFrom.y, m_Camera->m_LookFrom.z);
		}
		ImGui::End();

		// Demo window for reference
		bool show_demo_window = true;
		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);
	}

} // namespace Nova::App