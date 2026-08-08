#include "App/AppLayer.h"

#include <iostream>
#include <filesystem>
#include <algorithm>

#include "App/GameLayer.h"
#include "App/EditorLayer.h"

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

    void AppLayer::RequestPlay() {
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
    }

    void AppLayer::OnAttach() {
        g_AppLayer = this;

        GraphicsAPI api = Nova::Core::Application::Get().GetWindow().GetGraphicsAPI();

		Nova::Core::Renderer::RHI::RHI_SwapchainDesc swapDesc{};
        swapDesc.m_FramesInFlight = 3;
        swapDesc.m_CreateSurface = true;
        swapDesc.m_EnableSwapchain = true;
        swapDesc.m_PreferredPresentMode = Nova::Core::Renderer::RHI::RHI_PresentMode::Default;
		m_Renderer = Nova::Core::Renderer::RHI::IRenderer::Create(api, swapDesc);

		int initialW = 1280, initialH = 720;
        Nova::Core::Application::Get().GetWindow().GetWindowSize(initialW, initialH);
        if (initialW <= 0 || initialH <= 0) {
            initialW = 1280;
            initialH = 720;
        }
        m_ViewportSize = { static_cast<float>(initialW), static_cast<float>(initialH) };

		namespace RG = Nova::Core::Renderer::RHI;
        RG::RHI_RenderGraphBuilder fg;

        const uint32_t width = static_cast<uint32_t>(initialW);
        const uint32_t height = static_cast<uint32_t>(initialH);

        const auto backbuffer = fg.ImportTexture({
            width,
            height,
            RG::RHI_TextureFormat::RGBA8,
            RG::RHI_TextureUsage::ColorAttachment,
        }, RG::RHI_ResourceState::Present);

        // Editor viewport renders into offscreen textures sampled by ImGui.
        RG::RHI_TextureHandle color = fg.CreateTexture({
            width,
            height,
            RG::RHI_TextureFormat::RGBA8,
            RG::RHI_TextureUsage::ColorAttachment | RG::RHI_TextureUsage::Sampled,
        });

        RG::RHI_TextureHandle depth = fg.CreateTexture({
            width,
            height,
            RG::RHI_TextureFormat::Depth32,
            RG::RHI_TextureUsage::DepthAttachment | RG::RHI_TextureUsage::Sampled,
        });

        m_SceneColor = color;
        m_SceneDepth = depth;

        auto acquireShader = [](const std::filesystem::path& uri) {
            auto asset = AssetManager::Get().Acquire<ShaderAsset>(uri).GetAssetRef();
            if (!asset || !asset->Compile()) {
                NV_LOG_WARN(("Failed to compile shader asset: " + uri.generic_string()).c_str());
            }
            return asset;
        };

        auto gridVert = acquireShader("Editor://Shaders/Grid.vert.slang");
        auto gridFrag = acquireShader("Editor://Shaders/Grid.frag.slang");
        auto sceneVert = acquireShader("Engine://Shaders/Scene.vert.slang");
        auto sceneFrag = acquireShader("Engine://Shaders/Scene.frag.slang");
        auto normalsFrag = acquireShader("Engine://Shaders/NormalsDebug.frag.slang");
        auto positionsFrag = acquireShader("Engine://Shaders/PositionsDebug.frag.slang");
        auto vertexColorFrag = acquireShader("Engine://Shaders/VertexColor.frag.slang");
        auto depthFrag = acquireShader("Engine://Shaders/DepthDebug.frag.slang");

        m_GridShader = fg.RegisterShader({
            .m_Name = "Grid",
            .m_Vertex = gridVert,
            .m_Fragment = gridFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::FullscreenQuad,
            .m_AlphaBlend = true,
        });

        m_SceneShader = fg.RegisterShader({
            .m_Name = "Scene",
            .m_Vertex = sceneVert,
            .m_Fragment = sceneFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::Mesh,
        });
        m_NormalsShader = fg.RegisterShader({
            .m_Name = "NormalsDebug",
            .m_Vertex = sceneVert,
            .m_Fragment = normalsFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::Mesh,
        });
        m_PositionsShader = fg.RegisterShader({
            .m_Name = "PositionsDebug",
            .m_Vertex = sceneVert,
            .m_Fragment = positionsFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::Mesh,
        });
        m_VertexColorShader = fg.RegisterShader({
            .m_Name = "VertexColor",
            .m_Vertex = sceneVert,
            .m_Fragment = vertexColorFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::Mesh,
        });
        m_DepthShader = fg.RegisterShader({
            .m_Name = "DepthDebug",
            .m_Vertex = sceneVert,
            .m_Fragment = depthFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::Mesh,
        });

        fg.AddPass("Grid",
            [&](RG::RHI_PassBuilder& b) {
                b.Write(color);
                if (depth.IsValid())
                    b.Write(depth);
            },
            [this](RG::RHI_PassContext& ctx) {
                if (!m_ShowGrid)
                    return;
                auto* shader = ctx.GetShader(m_GridShader);
                if (!shader) return;
                const glm::vec3 resolution(ctx.GetRenderWidth(), ctx.GetRenderHeight(), 1.0f);
                shader->SetParameter("iResolution", resolution);
                ctx.DrawFullscreen(m_GridShader);
            });

        fg.AddPass("Scene",
            [&](RG::RHI_PassBuilder& b) {
                b.Read(color);
                b.Write(color);
                if (depth.IsValid()) {
                    b.Read(depth);
                    b.Write(depth);
                }
            },
            [this](RG::RHI_PassContext& ctx) {
                RenderScene(ctx);
            });

        fg.AddPass("UI",
            [&](RG::RHI_PassBuilder& b) {
                b.Write(backbuffer);
                b.PresentOnly();
            },
            [](RG::RHI_PassContext& /*ctx*/) {
                // ImGui draw data is rendered by ImGuiLayer during the present pass.
            });

        m_Renderer->SetRenderGraph(fg.Build(api));

        // Temporary directional light ConstantBuffer (Scene.frag.slang: ParameterBlock<AppResources> user).
        // Bound once at pipeline creation (GetShader builds the pipeline lazily), same idea as the
        // engine writeEngineBuffer path — no per-frame descriptor updates.
        m_Light.m_Direction = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f));
        m_Light.m_Intensity = 3.0f;
        m_Light.m_Color = glm::vec3(1.0f, 1.0f, 1.0f);
        {
            Nova::Core::Renderer::RHI::RHI_GpuBufferDesc lightDesc{};
            lightDesc.m_ElementSize = sizeof(DirectionalLight);
            lightDesc.m_ElementCount = 1;
            lightDesc.m_PerFrameInFlight = false;
            lightDesc.m_DebugName = "user.light";
            m_LightBuffer = m_Renderer->CreateConstantBuffer(lightDesc);
        }
        if (m_LightBuffer.IsValid()) {
            Nova::Core::Renderer::RHI::UpdateConstantBuffer(*m_Renderer, m_LightBuffer, m_Light);
            if (auto* graph = m_Renderer->GetRenderGraph()) {
                if (auto* sceneShader = graph->GetShader(m_SceneShader)) {
                    sceneShader->Resources().SetBuffer("user.light", m_LightBuffer, *m_Renderer);
                    sceneShader->CommitResources();
                }
            }
        }

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

		// CUBE
        auto cubeAsset = AssetManager::Get().Acquire<MeshAsset>("Engine://Primitives/Cube").GetAssetRef();
		cubeAsset->Load();
		entt::entity cubeEntity = m_Scene.CreateEntity("Cube");

		registry.emplace<TransformComponent>(cubeEntity,
            glm::vec3(0.0f, 0.5f, 0.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 1.0f)
        );
		{
			Nova::Core::Renderer::RHI::Material mat{};
			mat.m_BaseColor = glm::vec3(0.0f, 1.0f, 0.0f);
			registry.emplace<MeshRendererComponent>(cubeEntity, cubeAsset, mat);
		}

		// TORUS
        auto torusAsset = AssetManager::Get().Acquire<MeshAsset>("Engine://Primitives/Torus").GetAssetRef();
        torusAsset->Load();
        entt::entity torusEntity = m_Scene.CreateEntity("Torus");
        registry.emplace<TransformComponent>(torusEntity,
            glm::vec3(2.0f, 0.25f, 1.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 1.0f)
        );
        {
            Nova::Core::Renderer::RHI::Material mat{};
            mat.m_BaseColor = glm::vec3(1.0f, 0.5f, 0.0f);
            registry.emplace<MeshRendererComponent>(torusEntity, torusAsset, mat);
        }

        // SPHERE
        auto sphereAsset = AssetManager::Get().Acquire<MeshAsset>("Engine://Primitives/Sphere").GetAssetRef();
        sphereAsset->Load();
        entt::entity sphereEntity = m_Scene.CreateEntity("Sphere");
        registry.emplace<TransformComponent>(sphereEntity,
            glm::vec3(0.0f, 0.5f, -1.5f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 1.0f)
        );
        {
            Nova::Core::Renderer::RHI::Material mat{};
            mat.m_BaseColor = glm::vec3(0.0f, 0.0f, 1.0f);
            registry.emplace<MeshRendererComponent>(sphereEntity, sphereAsset, mat);
        }

		// GROUND
		auto planeAsset = AssetManager::Get().Acquire<MeshAsset>("Engine://Primitives/Plane").GetAssetRef();
		planeAsset->Load();
		entt::entity planeEntity = m_Scene.CreateEntity("Plane");

		registry.emplace<TransformComponent>(planeEntity,
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(10.0f, 10.0f, 10.0f)
        );

		{
			Nova::Core::Renderer::RHI::Material mat{};
			registry.emplace<MeshRendererComponent>(planeEntity, planeAsset, mat);
		}

    	UpdateCameraFromOrbit();
    }

    void AppLayer::OnDetach() {
        NV_ASSERT_MSG(m_Renderer, "Renderer is not initialized.");
        // Light buffer is owned by the renderer's GPU buffer pool; Destroy() waits idle then frees it.
        m_LightBuffer = {};
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

	Nova::Core::Renderer::RHI::RHI_ShaderHandle AppLayer::GetActiveSceneShader() const {
		switch (m_RenderDebugMode) {
            case RenderDebugMode::Normals:     return m_NormalsShader;
            case RenderDebugMode::Positions:   return m_PositionsShader;
            case RenderDebugMode::VertexColor: return m_VertexColorShader;
            case RenderDebugMode::Depth:       return m_DepthShader;
            case RenderDebugMode::Lit:
            default:
                return m_SceneShader;
        }
	}
	
	void AppLayer::OnBegin() {
		NV_ASSERT_MSG(m_Renderer, "Renderer is not initialized.");
		NV_ASSERT_MSG(m_Camera, "Camera is not initialized.");
		
        if (m_ViewportResizePending)
			ApplyPendingViewportResize();

		m_Renderer->BeginFrame();
		const glm::mat4 view = m_Camera->GetViewMatrix();
		const glm::mat4 proj = m_Camera->GetProjectionMatrix();
		const glm::mat4 viewProj = proj * view;
		const glm::mat4 invViewProj = glm::inverse(viewProj);

		auto setGlobals = [this, view, proj, viewProj, invViewProj](Nova::Core::Renderer::RHI::RHI_Shaders* shader) {
			if (!shader) return;
			shader->SetParameter("iTime", m_ElapsedTime);
			shader->SetParameter("iTimeDelta", m_DeltaTime);
			shader->SetParameter("iFrameRate", m_DeltaTime > 0.0f ? 1.0f / m_DeltaTime : 0.0f);
			shader->SetParameter("iFrame", static_cast<int>(m_FrameIndex++));
			shader->SetParameter("iResolution", glm::vec3(m_ViewportSize.x, m_ViewportSize.y, 1.0f));
			shader->SetParameter("view", view);
			shader->SetParameter("proj", proj);
			shader->SetParameter("viewProj", viewProj);
			shader->SetParameter("invViewProj", invViewProj);
		};

		if (auto* graph = m_Renderer->GetRenderGraph()) {
			setGlobals(graph->GetShader(m_GridShader));
			setGlobals(graph->GetShader(m_SceneShader));
			setGlobals(graph->GetShader(m_NormalsShader));
			setGlobals(graph->GetShader(m_PositionsShader));
			setGlobals(graph->GetShader(m_VertexColorShader));
			setGlobals(graph->GetShader(m_DepthShader));
		}
	}

	void AppLayer::OnRender() {
		NV_ASSERT_MSG(m_Renderer, "Renderer is not initialized.");
        NV_ASSERT_MSG(m_Camera, "Camera is not initialized.");
		m_Renderer->RenderFrame();
	}

	void AppLayer::OnEnd() {
		NV_ASSERT_MSG(m_Renderer, "Renderer is not initialized.");
		NV_ASSERT_MSG(m_Camera, "Camera is not initialized.");
		m_Renderer->EndFrame();
	}

	void AppLayer::RenderScene(Nova::Core::Renderer::RHI::RHI_PassContext& ctx) {
		NV_ASSERT_MSG(m_Renderer, "Renderer is not initialized.");
		NV_ASSERT_MSG(m_Camera, "Camera is not initialized.");

		const auto shaderHandle = GetActiveSceneShader();
        auto* sceneShader = ctx.GetShader(shaderHandle);
        if (!sceneShader) return;

		auto& registry = m_Scene.GetRegistry();
		auto viewMeshes = registry.view<TransformComponent, MeshRendererComponent>();
		for (auto entity : viewMeshes) {
			auto& tc = viewMeshes.get<TransformComponent>(entity);
			auto& mrc = viewMeshes.get<MeshRendererComponent>(entity);

			if (!mrc.m_MeshAsset || !mrc.m_MeshAsset->IsLoaded())
				continue;

			auto cpuMesh = mrc.m_MeshAsset->GetCPUMesh();
			if (!cpuMesh)
				continue;

			sceneShader->SetParameter("model", tc.GetTransform());
			sceneShader->SetParameter("u_CameraPos", m_Camera->m_LookFrom);

			sceneShader->SetParameter("base", mrc.m_Material.m_Base);
			sceneShader->SetParameter("baseColor", mrc.m_Material.m_BaseColor);
			sceneShader->SetParameter("diffuseRoughness", mrc.m_Material.m_DiffuseRoughness);
			sceneShader->SetParameter("metalness", mrc.m_Material.m_Metalness);
			sceneShader->SetParameter("metalColor", mrc.m_Material.m_MetalColor);
			sceneShader->SetParameter("specular", mrc.m_Material.m_Specular);
			sceneShader->SetParameter("specularColor", mrc.m_Material.m_SpecularColor);
			sceneShader->SetParameter("specularRoughness", mrc.m_Material.m_SpecularRoughness);
			sceneShader->SetParameter("specularIOR", mrc.m_Material.m_SpecularIOR);
			sceneShader->SetParameter("specularAnisotropy", mrc.m_Material.m_SpecularAnisotropy);
			sceneShader->SetParameter("specularRotation", mrc.m_Material.m_SpecularRotation);
			sceneShader->SetParameter("transmission", mrc.m_Material.m_Transmission);
			sceneShader->SetParameter("transmissionColor", mrc.m_Material.m_TransmissionColor);
			sceneShader->SetParameter("subsurface", mrc.m_Material.m_Subsurface);
			sceneShader->SetParameter("subsurfaceColor", mrc.m_Material.m_SubsurfaceColor);
			sceneShader->SetParameter("subsurfaceRadius", mrc.m_Material.m_SubsurfaceRadius);
			sceneShader->SetParameter("subsurfaceScale", mrc.m_Material.m_SubsurfaceScale);
			sceneShader->SetParameter("subsurfaceAnisotropy", mrc.m_Material.m_SubsurfaceAnisotropy);
			sceneShader->SetParameter("sheen", mrc.m_Material.m_Sheen);
			sceneShader->SetParameter("sheenColor", mrc.m_Material.m_SheenColor);
			sceneShader->SetParameter("sheenRoughness", mrc.m_Material.m_SheenRoughness);
			sceneShader->SetParameter("coat", mrc.m_Material.m_Coat);
			sceneShader->SetParameter("coatColor", mrc.m_Material.m_CoatColor);
			sceneShader->SetParameter("coatRoughness", mrc.m_Material.m_CoatRoughness);
			sceneShader->SetParameter("coatAnisotropy", mrc.m_Material.m_CoatAnisotropy);
			sceneShader->SetParameter("coatRotation", mrc.m_Material.m_CoatRotation);
			sceneShader->SetParameter("coatIOR", mrc.m_Material.m_CoatIOR);
			sceneShader->SetParameter("coatAffectColor", mrc.m_Material.m_CoatAffectColor);
			sceneShader->SetParameter("coatAffectRoughness", mrc.m_Material.m_CoatAffectRoughness);
			sceneShader->SetParameter("emission", mrc.m_Material.m_Emission);
			sceneShader->SetParameter("emissionColor", mrc.m_Material.m_EmissionColor);
			sceneShader->SetParameter("opacity", mrc.m_Material.m_Opacity);
			sceneShader->SetParameter("thinWalled", mrc.m_Material.m_ThinWalled);
			sceneShader->SetParameter("isOpaque", static_cast<int>(mrc.m_Material.m_IsOpaque));

			ctx.BindShader(shaderHandle);

			Nova::Core::Renderer::RHI::RHI_DrawIndexedCommand cmd{};
			cmd.m_Mesh = cpuMesh;
			cmd.m_Topology = Nova::Core::Renderer::RHI::RHI_PrimitiveTopology::Triangles;
			cmd.m_IndexType = Nova::Core::Renderer::RHI::RHI_IndexType::UInt32;
			cmd.m_IndexCount = static_cast<uint32_t>(cpuMesh->GetIndices().size());
			ctx.DrawIndexed(cmd);
		}
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
		m_Orbit.m_Pitch += delta.y * m_Orbit.m_RotateSensitivity;

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
		RequestViewportResize(static_cast<float>(e.GetWidth()), static_cast<float>(e.GetHeight()));
        return false;
	}

    bool AppLayer::OnImGuiPanelResize(ImGuiPanelResizeEvent& e) {
		RequestViewportResize(e.GetWidth(), e.GetHeight());
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

	void AppLayer::RequestViewportResize(float width, float height) {
		if (width <= 0.0f || height <= 0.0f)
            return;

        const int newW = static_cast<int>(std::lround(width));
        const int newH = static_cast<int>(std::lround(height));
        if (newW <= 0 || newH <= 0)
            return;

        const int pendingW = static_cast<int>(std::lround(m_PendingViewportSize.x));
        const int pendingH = static_cast<int>(std::lround(m_PendingViewportSize.y));
        if (newW == pendingW && newH == pendingH && m_ViewportResizePending)
            return;

        const int currentW = static_cast<int>(std::lround(m_ViewportSize.x));
        const int currentH = static_cast<int>(std::lround(m_ViewportSize.y));
        if (newW == currentW && newH == currentH && !m_ViewportResizePending)
            return;

        m_PendingViewportSize = { static_cast<float>(newW), static_cast<float>(newH) };
        m_ViewportResizePending = true;
	}

	void AppLayer::ApplyPendingViewportResize() {
        const int newW = static_cast<int>(std::lround(m_PendingViewportSize.x));
        const int newH = static_cast<int>(std::lround(m_PendingViewportSize.y));
        if (newW <= 0 || newH <= 0) {
            m_ViewportResizePending = false;
            return;
        }

        const int oldW = static_cast<int>(std::lround(m_ViewportSize.x));
        const int oldH = static_cast<int>(std::lround(m_ViewportSize.y));
        if (newW == oldW && newH == oldH) {
            m_ViewportResizePending = false;
            return;
        }

        m_ViewportSize = { static_cast<float>(newW), static_cast<float>(newH) };
        m_Renderer->Resize(newW, newH);

        if (m_Camera)
            m_Camera->m_AspectRatio = static_cast<float>(newW) / static_cast<float>(newH);

        m_ViewportResizePending = false;
    }

} // namespace Nova::App