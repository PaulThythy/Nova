#include "App/VkLayer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <random>

namespace Nova::App {

	constexpr size_t kCubeInstanceCount = 250;
	constexpr float kSceneRadius = 14.0f;
	constexpr float kSceneHeight = 4.0f;
	constexpr float kCenterExclusionRadius = 1.5f;
	constexpr float kAxisLength = 1.0f;
	constexpr float kAxisThickness = 0.05f;

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

		CreateInstancedCubeScene();
		UpdateCameraFromOrbit();
	}

	void VkLayer::OnDetach() {
		NV_ASSERT_MSG(m_Renderer, "Renderer is not initialized.");
		m_Renderer->Destroy();
		m_Renderer.reset();
	}
	
	void VkLayer::OnUpdate(float dt) {
		m_DeltaTime = dt;
		m_ElapsedTime += dt;

		for (auto& cube : m_Cubes) {
			cube.m_RotationAngle += cube.m_RotationSpeed * dt;
		}

		UpdateCubeInstances();
	}

	void VkLayer::OnBegin() {
		NV_ASSERT_MSG(m_Renderer, "Renderer is not initialized.");

		if (m_ViewportResizePending) {
			m_ViewportResizePending = false;
			if (m_PendingViewportSize.x > 0 && m_PendingViewportSize.y > 0) {
				m_Renderer->Resize(m_PendingViewportSize.x, m_PendingViewportSize.y);
			}
		}

		m_Renderer->BeginFrame();
	}
	
	void VkLayer::OnRender() {
		NV_ASSERT_MSG(m_Renderer, "Renderer is not initialized.");
		NV_ASSERT_MSG(m_Camera, "Camera is not initialized.");

		auto& registry = m_Scene.GetRegistry();

		const glm::mat4 view = m_Camera->GetViewMatrix();
		const glm::mat4 proj = m_Camera->GetProjectionMatrix();

		m_Renderer->BeginScene(view, proj);

		if (auto* shader = m_Renderer->GetShader()) {
			shader->SetParameter("iTime", m_ElapsedTime);
			shader->SetParameter("iTimeDelta", m_DeltaTime);
			shader->SetParameter("iFrameRate", m_DeltaTime > 0.0f ? 1.0f / m_DeltaTime : 0.0f);
			shader->SetParameter("iFrame", static_cast<int>(m_FrameIndex++));
			shader->SetParameter("iResolution", glm::vec3(m_ViewportSize.x, m_ViewportSize.y, 1.0f));
		}

		if (m_CubeMeshAsset && m_CubeMeshAsset->IsLoaded() && !m_InstanceData.empty()) {
			auto gpuMesh = m_CubeMeshAsset->GetGPUMesh();
			if (gpuMesh) {
				Nova::Core::Renderer::RHI::RHI_DrawIndexedCommand cmd{};
				cmd.m_Mesh = gpuMesh;
				cmd.m_Topology = Nova::Core::Renderer::RHI::RHI_PrimitiveTopology::Triangles;
				cmd.m_IndexType = Nova::Core::Renderer::RHI::RHI_IndexType::UInt32;
				cmd.m_IndexCount = static_cast<uint32_t>(gpuMesh->GetIndices().size());
				cmd.m_InstanceCount = static_cast<uint32_t>(m_InstanceData.size());

				if (auto* shader = m_Renderer->GetShader()) {
					shader->SetParameter("u_UseInstancing", 1);
					shader->SetInstanceData(m_InstanceData);
					shader->SetParameter("u_Color", glm::vec4(1.0f));
				}

				m_Renderer->SetModelMatrix(glm::mat4(1.0f));
				m_Renderer->DrawIndexed(cmd);

				if (auto* shader = m_Renderer->GetShader()) {
					shader->SetParameter("u_UseInstancing", 0);
				}
			}
		}

		// ECS traversal: draw all entities that have a transform and a mesh.
		auto viewMeshes = registry.view<TransformComponent, MeshComponent>();
		for (auto entity : viewMeshes) {
			auto& tc = viewMeshes.get<TransformComponent>(entity);
			auto& mc = viewMeshes.get<MeshComponent>(entity);

			if (!mc.m_MeshAsset || !mc.m_MeshAsset->IsLoaded())
				continue;

			auto gpuMesh = mc.m_MeshAsset->GetGPUMesh();
			if (!gpuMesh)
				continue;

			Nova::Core::Renderer::RHI::RHI_DrawIndexedCommand cmd{};
			cmd.m_Mesh = gpuMesh;
			cmd.m_Topology = Nova::Core::Renderer::RHI::RHI_PrimitiveTopology::Triangles;
			cmd.m_IndexType = Nova::Core::Renderer::RHI::RHI_IndexType::UInt32;
			cmd.m_IndexCount = static_cast<uint32_t>(gpuMesh->GetIndices().size());

			m_Renderer->SetModelMatrix(tc.GetTransform());
			m_Renderer->GetShader()->SetParameter("u_UseInstancing", 0);
			m_Renderer->GetShader()->SetParameter("u_Color", glm::vec4(1.0f));

			m_Renderer->DrawIndexed(cmd);
		}

	}

	void VkLayer::CreateInstancedCubeScene() {
		m_CubeMeshAsset = AssetManager::Get().Acquire<MeshAsset>("Engine://Primitives/Cube").GetAssetRef();
		NV_ASSERT_MSG(m_CubeMeshAsset, "Failed to acquire the cube mesh asset.");
		if (!m_CubeMeshAsset) {
			return;
		}

		m_CubeMeshAsset->Load();

		m_Cubes.clear();
		m_Cubes.reserve(kCubeInstanceCount);
		m_InstanceData.reserve(kCubeInstanceCount);

		std::mt19937 rng(1337u);
		std::uniform_real_distribution<float> positionDist(-kSceneRadius, kSceneRadius);
		std::uniform_real_distribution<float> heightDist(-kSceneHeight, kSceneHeight);
		std::uniform_real_distribution<float> axisDist(-1.0f, 1.0f);
		std::uniform_real_distribution<float> speedDist(0.15f, 0.45f);
		std::uniform_real_distribution<float> scaleDist(0.45f, 1.35f);
		std::uniform_real_distribution<float> colorDist(0.2f, 1.0f);

		for (size_t i = 0; i < kCubeInstanceCount; ++i) {
			AnimatedCube cube{};
			do {
				cube.m_Position = {
					positionDist(rng),
					heightDist(rng),
					positionDist(rng)
				};
			} while (glm::length(cube.m_Position) < kCenterExclusionRadius);

			glm::vec3 axis{
				axisDist(rng),
				axisDist(rng),
				axisDist(rng)
			};
			if (glm::length(axis) < 0.001f) {
				axis = glm::vec3(0.0f, 1.0f, 0.0f);
			}

			cube.m_RotationAxis = glm::normalize(axis);
			cube.m_RotationSpeed = speedDist(rng);
			cube.m_RotationAngle = glm::radians(positionDist(rng) * 10.0f);
			cube.m_Scale = scaleDist(rng);
			cube.m_Color = glm::vec4(
				colorDist(rng),
				colorDist(rng),
				colorDist(rng),
				1.0f
			);
			m_Cubes.push_back(cube);
		}

		m_Orbit.m_Target = glm::vec3(0.0f);
		UpdateCubeInstances();
	}

	void VkLayer::UpdateCubeInstances() {
		m_InstanceData.clear();
		m_InstanceData.reserve(m_Cubes.size() + 3);

		for (const auto& cube : m_Cubes) {
			glm::mat4 model = glm::translate(glm::mat4(1.0f), cube.m_Position);
			model = glm::rotate(model, cube.m_RotationAngle, cube.m_RotationAxis);
			model = glm::scale(model, glm::vec3(cube.m_Scale));

			RHI::SSBO_InstanceData instance{};
			instance.model = model;
			instance.color = cube.m_Color;
			m_InstanceData.push_back(instance);
		}

		m_Orbit.m_Target = glm::vec3(0.0f);

		const auto appendAxisMarker = [this](const glm::vec3& translation, const glm::vec3& scale, const glm::vec4& color) {
			RHI::SSBO_InstanceData instance{};
			instance.model = glm::translate(glm::mat4(1.0f), translation)
				* glm::scale(glm::mat4(1.0f), scale);
			instance.color = color;
			m_InstanceData.push_back(instance);
		};

		appendAxisMarker(
			glm::vec3(kAxisLength * 0.5f, 0.0f, 0.0f),
			glm::vec3(kAxisLength, kAxisThickness, kAxisThickness),
			glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)
		);
		appendAxisMarker(
			glm::vec3(0.0f, kAxisLength * 0.5f, 0.0f),
			glm::vec3(kAxisThickness, kAxisLength, kAxisThickness),
			glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)
		);
		appendAxisMarker(
			glm::vec3(0.0f, 0.0f, kAxisLength * 0.5f),
			glm::vec3(kAxisThickness, kAxisThickness, kAxisLength),
			glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)
		);
	}

	void VkLayer::OnEnd() {
		NV_ASSERT_MSG(m_Renderer, "Renderer is not initialized.");
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
		// Clamp pitch to avoid gimbal singularities (flip at ±90°).
		const float maxPitch = glm::radians(89.0f);
		m_Orbit.m_Pitch = std::clamp(m_Orbit.m_Pitch, -maxPitch, maxPitch);
		m_Orbit.m_Distance = std::max(0.2f, m_Orbit.m_Distance);

		const float cp = std::cos(m_Orbit.m_Pitch);

		// yaw=0 places the camera on the +Z axis.
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
		// If ImGui is consuming mouse input (window drag, slider, etc.), ignore it for the camera.
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

		// If ImGui has captured the mouse, skip orbit input.
		if (ImGui::GetIO().WantCaptureMouse) {
			// Keep the last position up-to-date to avoid a position jump when the cursor re-enters the viewport.
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
		if (ImGui::Begin("Statistics", nullptr, ImGuiWindowFlags_None)) {
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
			
			ImGui::Separator();
			ImGui::Text("Instancing");
			ImGui::Text("Cubes: %zu", m_Cubes.size());
			ImGui::Text("Orbit Target: (0.00, 0.00, 0.00)");
		}
		ImGui::End();

		// Viewport panel for rendering output (OpenGL or Vulkan).
		// OpenGL: FBO color texture. Vulkan: offscreen viewport image via VkDescriptorSet.
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		if (ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
			const ImVec2 avail = ImGui::GetContentRegionAvail();
			if (avail.x > 0.0f && avail.y > 0.0f && m_Camera) {
				const glm::vec2 newSize{ avail.x, avail.y };
				if (newSize != m_ViewportSize) {
					m_ViewportSize = newSize;

					m_Camera->m_AspectRatio = newSize.x / newSize.y;
					m_PendingViewportSize = {
						std::max(1, static_cast<int>(newSize.x)),
						std::max(1, static_cast<int>(newSize.y))
					};
					m_ViewportResizePending = true;
				}
		
				if (m_Renderer) {
					if (void* textureId = m_Renderer->GetViewportTextureID()) {
						// OpenGL FBOs have Y=0 at the bottom, so a V-flip is required.
						// Vulkan/Metal/DX have Y=0 at the top: no flip needed.
						const GraphicsAPI api = Nova::Core::Application::Get().GetWindow().GetGraphicsAPI();
						const bool needsVFlip = (api == GraphicsAPI::OpenGL);
						const ImVec2 uv0 = needsVFlip ? ImVec2(0, 1) : ImVec2(0, 0);
						const ImVec2 uv1 = needsVFlip ? ImVec2(1, 0) : ImVec2(1, 1);
						ImGui::Image(textureId, avail, uv0, uv1);
					}
				}
			}
		}
		ImGui::End();
		ImGui::PopStyleVar();

		// Demo window for reference
		bool show_demo_window = true;
		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);
	}

} // namespace Nova::App
