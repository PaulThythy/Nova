#include "App/AppLayer.h"

#include <iostream>
#include <filesystem>

//#include "App/GameLayer.h"
//#include "App/EditorLayer.h"

namespace Nova::App {

    AppLayer* g_AppLayer = nullptr;

    AppLayer::~AppLayer() = default;

    void AppLayer::SetupDockSpace(ImGuiID dockspace_id) {
        static bool s_DockInitialized = false;
        if (s_DockInitialized)
            return;

        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_None);
        ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

        ImGuiID dock_down = 0;
        ImGuiID dock_center = 0;
        ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.19f, &dock_down, &dock_center);

        ImGuiID dock_right = 0;
        ImGuiID dock_left = 0;
        ImGui::DockBuilderSplitNode(dock_center, ImGuiDir_Right, 0.27f, &dock_right, &dock_left);

        ImGuiID dock_right_top = 0;
        ImGuiID dock_right_bottom = 0;
        ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Up, 0.50f, &dock_right_top, &dock_right_bottom);

        // Dock windows
        ImGui::DockBuilderDockWindow(m_Scene.GetName().c_str(), dock_left);
        ImGui::DockBuilderDockWindow("Hierarchy", dock_right_top);
        ImGui::DockBuilderDockWindow("Inspector", dock_right_bottom);
        ImGui::DockBuilderDockWindow("Asset Browser", dock_down);

        ImGui::DockBuilderFinish(dockspace_id);
        s_DockInitialized = true;
    }

    void AppLayer::OnEvent(Event& e) {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& ev) { return OnMouseButtonPressed(ev); });
		dispatcher.Dispatch<MouseButtonReleasedEvent>([this](MouseButtonReleasedEvent& ev) { return OnMouseButtonReleased(ev); });
		dispatcher.Dispatch<MouseMovedEvent>([this](MouseMovedEvent& ev) { return OnMouseMoved(ev); });
		dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent& ev) { return OnMouseScrolled(ev); });
		dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) { return OnWindowResized(ev); });
		dispatcher.Dispatch<ImGuiPanelResizeEvent>([this](ImGuiPanelResizeEvent& ev) { return OnImGuiPanelResize(ev); });
    }

    /*void AppLayer::RequestPlay() {
        if (m_SceneState == SceneState::Play)
            return;

        if (!m_EditorLayer) {
            std::cerr << "[AppLayer] Cannot Play: EditorLayer not registered.\n";
            return;
        }

        // Replace the current EditorLayer with GameLayer (keep AppLayer alive for UI).
        Nova::Core::Application::Get().GetLayerStack().QueueLayerTransition<GameLayer>(m_EditorLayer);
        std::cout << "AppLayer: Transition to GameLayer requested.\n";

        SetSceneState(SceneState::Play);
    }

    void AppLayer::RequestStop() {
        if (m_SceneState == SceneState::Edit)
            return;

        if (!m_GameLayer) {
            std::cerr << "[AppLayer] Cannot Stop: GameLayer not registered.\n";
            return;
        }

        // Replace the current GameLayer with EditorLayer (keep AppLayer alive for UI).
        Nova::Core::Application::Get().GetLayerStack().QueueLayerTransition<EditorLayer>(m_GameLayer);
        std::cout << "AppLayer: Transition to EditorLayer requested.\n";

        SetSceneState(SceneState::Edit);
    }*/

    void AppLayer::OnAttach() {
        g_AppLayer = this;

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

        auto cubeAsset = AssetManager::Get().Acquire<MeshAsset>("Engine://Primitives/Cube").GetAssetRef();
		cubeAsset->Load();
		entt::entity cubeEntity = m_Scene.CreateEntity("Cube");

		registry.emplace<TransformComponent>(cubeEntity,
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 1.0f)
        );
		registry.emplace<MeshComponent>(cubeEntity, cubeAsset);

		UpdateCameraAspectFromWindow();
    	UpdateCameraFromOrbit();
    }

    void AppLayer::OnDetach() {
        NV_ASSERT_MSG(m_Renderer, "Renderer is not initialized.");
		m_Renderer->Destroy();
		m_Renderer.reset();

        m_Scene.Clear();

        if (g_AppLayer == this)
            g_AppLayer = nullptr;
    }

    void AppLayer::OnUpdate(float dt) {
        m_DeltaTime = dt;
        m_ElapsedTime += dt;
    }

    void AppLayer::OnBegin() {
		NV_ASSERT_MSG(m_Renderer, "Renderer is not initialized.");

		if (m_ViewportResizePending) {
			m_ViewportResizePending = false;
			if (m_PendingViewportSize.x > 0 && m_PendingViewportSize.y > 0) {
				m_Renderer->Resize(m_PendingViewportSize.x, m_PendingViewportSize.y);
			}
		}

		m_Renderer->BeginFrame();
	}
	
	void AppLayer::OnRender() {
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

    void AppLayer::OnEnd() {
		NV_ASSERT_MSG(m_Renderer, "Renderer is not initialized.");
		m_Renderer->EndFrame();
	}

    void AppLayer::OnImGuiRender() {
        UI::Panels::MainMenuBar::Render();

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGuiWindowFlags host_flags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_NoDocking;

        ImGui::Begin("Nova Editor", nullptr, host_flags);
        ImGui::PopStyleVar(3);

        ImGuiID dockspace_id = ImGui::GetID("NovaDockspace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

        SetupDockSpace(dockspace_id);
        ImGui::End();

        // Docked windows
        UI::Panels::ScenePanel::Render(m_Scene.GetName());
        UI::Panels::HierarchyPanel::Render();
        UI::Panels::InspectorPanel::Render();
        UI::Panels::AssetBrowserPanel::Render();
    }

    bool AppLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e) {
		// Only start orbit rotation when the mouse is inside the rendered viewport.
		if (!m_ViewportHovered)
			return false;

		if (e.GetMouseButton() == 1) {
			m_Orbit.m_IsRotating = true;
			m_Orbit.m_HasLastMousePos = false;
			return true;
		}
		return false;
	}

	bool AppLayer::OnMouseButtonReleased(MouseButtonReleasedEvent& e) {
		// Always stop rotation, even if the mouse left the viewport while dragging.
		if (e.GetMouseButton() == 1) {
			m_Orbit.m_IsRotating = false;
			return true;
		}
		return false;
	}

	bool AppLayer::OnMouseMoved(MouseMovedEvent& e) {
		const glm::vec2 mousePos{ e.GetX(), e.GetY() };

		if (!m_Orbit.m_IsRotating) {
			// Track position continuously so there is no jump when rotation starts.
			m_Orbit.m_LastMousePos = mousePos;
			m_Orbit.m_HasLastMousePos = true;
			return false;
		}

		// While rotating, continue even if the mouse left the viewport (standard editor behaviour).
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

	bool AppLayer::OnMouseScrolled(MouseScrolledEvent& e) {
		// Only zoom when the mouse is inside the rendered viewport.
		if (!m_ViewportHovered)
			return false;

		m_Orbit.m_Distance -= e.GetYOffset() * m_Orbit.m_ZoomSensitivity;
		UpdateCameraFromOrbit();
		return true;
	}

	bool AppLayer::OnWindowResized(WindowResizeEvent& e) {
		if (e.GetWidth() <= 0 || e.GetHeight() <= 0) return false;
		m_Camera->m_AspectRatio = static_cast<float>(e.GetWidth()) / static_cast<float>(e.GetHeight());
		return false;
	}

    bool AppLayer::OnImGuiPanelResize(ImGuiPanelResizeEvent& e) {
		const float w = e.GetWidth();
		const float h = e.GetHeight();
		if (w <= 0.0f || h <= 0.0f) return false;

		m_ViewportSize = { w, h };

		if (m_Camera)
			m_Camera->m_AspectRatio = w / h;

		m_PendingViewportSize   = { w, h };
		m_ViewportResizePending = true;
		return false;
	}

    void AppLayer::UpdateCameraFromOrbit() {
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

	void AppLayer::UpdateCameraAspectFromWindow() {
		SDL_Window* window = Nova::Core::Application::Get().GetWindow().GetSDLWindow();
		int w = 0, h = 0;
		SDL_GetWindowSizeInPixels(window, &w, &h);

		if (w > 0 && h > 0) {
			m_Camera->m_AspectRatio = static_cast<float>(w) / static_cast<float>(h);
		}
	}

} // namespace Nova::App
