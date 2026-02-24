#include "App/VkLayer.h"

namespace Nova::App {

	VkLayer::~VkLayer() = default;

	void VkLayer::OnAttach() {
		GraphicsAPI api = Nova::Core::Application::Get().GetWindow().GetGraphicsAPI();
		m_Renderer = Nova::Core::Renderer::RHI::IRenderer::Create(api);

		// camera setup
		m_Camera = std::make_shared<Renderer::Graphics::Camera>(
            glm::vec3(5.0f, 5.0f, 5.0f),               // lookFrom
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

		UpdateCameraAspectFromWindow();
    	UpdateCameraFromOrbit();

		// load asset
		auto cubeAsset = AssetManager::Get().Acquire<MeshAsset>("Engine://Primitives/Cube").GetAssetRef();
		cubeAsset->Load();
		entt::entity cubeEntity = m_Scene.CreateEntity("Cube");

		registry.emplace<TransformComponent>(cubeEntity,
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 1.0f)
        );
		registry.emplace<MeshComponent>(cubeEntity, cubeAsset);
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
		if (!m_Renderer || !m_Camera) return;

		auto& registry = m_Scene.GetRegistry();

		// View
		const glm::mat4 view = glm::lookAt(
			m_Camera->m_LookFrom,
			m_Camera->m_LookAt,
			m_Camera->m_Up
		);

		// Projection
		glm::mat4 proj{1.0f};
		if (m_Camera->m_IsPerspective) {
			// IMPORTANT: essaie d'unifier ta convention (ZO vs NO) entre GL/VK.
			// Si tu utilises glClipControl(..., GL_ZERO_TO_ONE), une projection ZO est idéale.
			proj = glm::perspective(
				glm::radians(m_Camera->m_FOV),
				m_Camera->m_AspectRatio,
				m_Camera->m_NearPlane,
				m_Camera->m_FarPlane
			);
		} else {
			// à adapter selon ta Camera (ortho)
			proj = glm::mat4(1.0f);
		}

		// Parcours ECS : tous les objets rendables
		auto viewMeshes = registry.view<TransformComponent, MeshComponent>();
		for (auto entity : viewMeshes) {
			auto& tc = viewMeshes.get<TransformComponent>(entity);
			auto& mc = viewMeshes.get<MeshComponent>(entity);

			if (!mc.m_MeshAsset || !mc.m_MeshAsset->IsLoaded())
				continue;

			auto gpuMesh = mc.m_MeshAsset->GetGPUMesh();
			if (!gpuMesh)
				continue;

			const bool hasIndices = !gpuMesh->GetIndices().empty();

			if (hasIndices) {
				Nova::Core::Renderer::RHI::RHI_DrawIndexedCommand cmd{};
				cmd.m_Mesh = gpuMesh;
				cmd.m_Model = tc.GetTransform();
				cmd.m_View = view;
				cmd.m_Proj = proj;
				cmd.m_Topology = Nova::Core::Renderer::RHI::RHI_PrimitiveTopology::Triangles;
				cmd.m_IndexType = Nova::Core::Renderer::RHI::RHI_IndexType::UInt32;
				cmd.m_IndexCount = static_cast<uint32_t>(gpuMesh->GetIndices().size());

				m_Renderer->DrawIndexed(cmd);
			} else {
				Nova::Core::Renderer::RHI::RHI_DrawCommand cmd{};
				cmd.m_Mesh = gpuMesh;
				cmd.m_Model = tc.GetTransform();
				cmd.m_View = view;
				cmd.m_Proj = proj;
				cmd.m_Topology = Nova::Core::Renderer::RHI::RHI_PrimitiveTopology::Triangles;
				cmd.m_VertexCount = static_cast<uint32_t>(gpuMesh->GetVertices().size());

				m_Renderer->Draw(cmd);
			}
		}
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
		// Si ImGui utilise la souris (drag d'une fenêtre, slider, etc.), on ignore pour la caméra
		if (ImGui::GetIO().WantCaptureMouse) {
			return false;
		}

		if (e.GetMouseButton() == 1 /* Right placeholder */) {
			m_Orbit.m_IsRotating = true;
			m_Orbit.m_HasLastMousePos = false;
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
		const glm::vec2 mousePos{ e.GetX(), e.GetY() };

		// Si ImGui capture la souris, on n'applique pas d'orbite
		if (ImGui::GetIO().WantCaptureMouse) {
			// On garde la dernière position à jour pour éviter un "saut" quand on revient dans la vue
			m_Orbit.m_LastMousePos = mousePos;
			m_Orbit.m_HasLastMousePos = true;
			return false;
		}

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

		m_Orbit.m_Yaw   -= delta.x * m_Orbit.m_RotateSensitivity;
		m_Orbit.m_Pitch -= delta.y * m_Orbit.m_RotateSensitivity;

		UpdateCameraFromOrbit();
		return true;
	}

	bool VkLayer::OnMouseScrolled(MouseScrolledEvent& e) {
		if (ImGui::GetIO().WantCaptureMouse) {
			return false;
		}

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
		ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);
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