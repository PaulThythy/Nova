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

        RG::RHI_TextureHandle shadowMaps = fg.CreateTexture({
            Nova::Core::Renderer::RHI::SHADOW_MAP_RESOLUTION,
            Nova::Core::Renderer::RHI::SHADOW_MAP_RESOLUTION,
            RG::RHI_TextureFormat::Depth32,
            RG::RHI_TextureUsage::DepthAttachment | RG::RHI_TextureUsage::Sampled,
            Nova::Core::Renderer::RHI::MAX_SHADOW_MAPS,
            /*m_ResizeWithViewport*/ false,
            /*m_ComparisonSampler*/ true,
        });

        m_SceneColor = color;
        m_SceneDepth = depth;
        m_ShadowMaps = shadowMaps;

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
        auto shadowVert = acquireShader("Engine://Shaders/Shadow.vert.slang");
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
        m_ShadowShader = fg.RegisterShader({
            .m_Name = "Shadow",
            .m_Vertex = shadowVert,
            .m_Fragment = nullptr,
            .m_VertexLayout = RG::RHI_VertexLayout::Mesh,
            .m_AlphaBlend = false,
            .m_DepthTest = true,
            .m_DepthWrite = true,
            .m_DepthOnly = true,
            .m_CullMode = RG::RHI_CullMode::Front,
            .m_DepthBiasConstant = 0.5f,
            .m_DepthBiasSlope = 1.0f,
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

        fg.AddPass("Shadow",
            [&](RG::RHI_PassBuilder& b) {
                b.Write(shadowMaps);
            },
            [this](RG::IPassContext& ctx) {
                RenderShadowPass(ctx);
            });

        fg.AddPass("Grid",
            [&](RG::RHI_PassBuilder& b) {
                b.Write(color);
                if (depth.IsValid())
                    b.Write(depth);
            },
            [this](RG::IPassContext& ctx) {
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
                b.Read(shadowMaps);
            },
            [this](RG::IPassContext& ctx) {
                RenderScene(ctx);
            });

        fg.AddPass("UI",
            [&](RG::RHI_PassBuilder& b) {
                b.Write(backbuffer);
                b.PresentOnly();
            },
            [](RG::IPassContext& /*ctx*/) {
                // ImGui draw data is rendered by ImGuiLayer during the present pass.
            });

        m_Renderer->SetRenderGraph(fg.Build(api));

        if (auto* graph = m_Renderer->GetRenderGraph())
            graph->BindEngineShadowMaps(m_ShadowMaps);

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

        // Directional light (shadow casting)
        {
            entt::entity dirLightEntity = m_Scene.CreateEntity("DirectionalLight");
            registry.emplace<TransformComponent>(dirLightEntity,
                glm::vec3(0.0f, 8.0f, 0.0f),
                glm::vec3(glm::radians(-45.0f), glm::radians(45.0f), 0.0f),
                glm::vec3(1.0f));
            auto dirLight = std::make_shared<Light>();
            dirLight->m_Type = LightType::Directional;
            dirLight->m_Color = glm::vec3(1.0f);
            dirLight->m_Intensity = 3.0f;
            dirLight->m_Direction = glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f));
            dirLight->m_LightShadow = true;
            dirLight->m_ShadowBiasConstant = 0.5f;
            dirLight->m_ShadowBiasSlope = 1.0f;
            dirLight->m_ShadowNormalBias = 0.012f;
            registry.emplace<LightComponent>(dirLightEntity, dirLight);
        }

        // Spot light demo (shadow casting)
        /*{
            entt::entity spotEntity = m_Scene.CreateEntity("SpotLight");
            registry.emplace<TransformComponent>(spotEntity,
                glm::vec3(2.0f, 6.0f, 2.0f),
                glm::vec3(glm::radians(-60.0f), glm::radians(-20.0f), 0.0f),
                glm::vec3(1.0f));
            auto spot = std::make_shared<Light>();
            spot->m_Type = LightType::Spot;
            spot->m_Color = glm::vec3(1.0f, 1.0f, 1.0f);
            spot->m_Intensity = 8.0f;
            spot->m_Direction = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.2f));
            spot->m_Range = 20.0f;
            spot->m_InnerCone = 15.0f;
            spot->m_OuterCone = 30.0f;
            spot->m_LightShadow = true;
            // Perspective shadows amplify bias into peter panning — keep these lower than dir.
            spot->m_ShadowBiasConstant = 0.2f;
            spot->m_ShadowBiasSlope = 0.4f;
            spot->m_ShadowNormalBias = 0.003f;
            registry.emplace<LightComponent>(spotEntity, spot);
        }*/

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
        UploadLights();

		const glm::mat4 view = m_Camera->GetViewMatrix();
		const glm::mat4 proj = m_Camera->GetProjectionMatrix();
		const glm::mat4 viewProj = proj * view;
		const glm::mat4 invViewProj = glm::inverse(viewProj);
        const int lightCount = static_cast<int>(m_GpuLights.size());

		auto setGlobals = [this, view, proj, viewProj, invViewProj, lightCount](Nova::Core::Renderer::RHI::IShaders* shader) {
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
            shader->SetParameter("lightCount", lightCount);
		};

		if (auto* graph = m_Renderer->GetRenderGraph()) {
			setGlobals(graph->GetShader(m_GridShader));
			setGlobals(graph->GetShader(m_SceneShader));
            setGlobals(graph->GetShader(m_ShadowShader));
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

	void AppLayer::RenderScene(Nova::Core::Renderer::RHI::IPassContext& ctx) {
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

        if (auto* graph = m_Renderer->GetRenderGraph()) graph->BindEngineShadowMaps(m_ShadowMaps);

        if (m_Camera)
            m_Camera->m_AspectRatio = static_cast<float>(newW) / static_cast<float>(newH);

        m_ViewportResizePending = false;
    }

    void AppLayer::UploadLights() {
        m_GpuLights.clear();
        auto& registry = m_Scene.GetRegistry();
        auto view = registry.view<TransformComponent, LightComponent>();

        int nextShadow = 0;
        for (auto entity : view) {
            if (m_GpuLights.size() >= Nova::Core::Renderer::RHI::MAX_LIGHTS)
                break;

            auto& tc = view.get<TransformComponent>(entity);
            auto& lc = view.get<LightComponent>(entity);
            if (!lc.m_Light)
                continue;

            const Light& light = *lc.m_Light;
            const glm::vec3 position = tc.m_Translation;
            const glm::vec3 travelDir = glm::length(light.m_Direction) > 1e-6f
                ? glm::normalize(light.m_Direction)
                : glm::vec3(0.0f, -1.0f, 0.0f);

            Nova::Core::Renderer::RHI::LightGPU gpu{};
            gpu.m_Type = static_cast<int>(light.m_Type);
            gpu.m_Intensity = light.m_Intensity;
            gpu.m_Color = light.m_Color;
            gpu.m_Range = light.m_Range;
            gpu.m_Direction = travelDir;
            gpu.m_InnerConeCos = light.InnerCos();
            gpu.m_Position = position;
            gpu.m_OuterConeCos = light.OuterCos();
            gpu.m_ShadowBiasConstant = light.m_ShadowBiasConstant;
            gpu.m_ShadowBiasSlope = light.m_ShadowBiasSlope;
            gpu.m_ShadowNormalBias = light.m_ShadowNormalBias;
            gpu.m_CastShadow = 0;
            gpu.m_ShadowMapIndex = -1;

            const bool canShadow = light.m_LightShadow
                && (light.m_Type == LightType::Directional || light.m_Type == LightType::Spot)
                && nextShadow < static_cast<int>(Nova::Core::Renderer::RHI::MAX_SHADOW_MAPS);
            if (canShadow) {
                gpu.m_CastShadow = 1;
                gpu.m_ShadowMapIndex = nextShadow++;
                gpu.m_LightViewProj = BuildLightViewProj(
                    light.m_Type, position, travelDir, light.m_Range, light.m_OuterCone,
                    Nova::Core::Renderer::RHI::SHADOW_DIR_ORTHO_HALF_EXTENT);
            }

            m_GpuLights.push_back(gpu);
        }

        auto* graph = m_Renderer ? m_Renderer->GetRenderGraph() : nullptr;
        const auto* engine = graph ? graph->GetEngineParameterBlock() : nullptr;
        if (!engine || !engine->m_Lights.IsValid())
            return;

        if (!m_GpuLights.empty()) {
            m_Renderer->UpdateGpuBuffer(
                engine->m_Lights,
                m_GpuLights.data(),
                sizeof(Nova::Core::Renderer::RHI::LightGPU) * m_GpuLights.size(),
                0);
        }
    }

    void AppLayer::RenderShadowPass(Nova::Core::Renderer::RHI::IPassContext& ctx) {
        if (!m_Renderer || !m_ShadowMaps.IsValid())
            return;

        // Clear every layer so unused maps stay at far depth and the image layout is valid
        // even when there are zero shadow casters this frame.
        for (uint32_t layer = 0; layer < Nova::Core::Renderer::RHI::MAX_SHADOW_MAPS; ++layer) {
            ctx.BeginDepthLayer(m_ShadowMaps, layer, true);
            ctx.EndDepthLayer();
        }

        auto* shadowShader = ctx.GetShader(m_ShadowShader);
        if (!shadowShader)
            return;

        auto& registry = m_Scene.GetRegistry();
        auto meshView = registry.view<TransformComponent, MeshRendererComponent>();

        for (const auto& light : m_GpuLights) {
            if (!light.m_CastShadow || light.m_ShadowMapIndex < 0)
                continue;

            ctx.BeginDepthLayer(m_ShadowMaps, static_cast<uint32_t>(light.m_ShadowMapIndex), true);
            // Same idea as Light::ShadowAngleBiasFactor: less raster bias when light is grazing.
            const float dirLen2 = glm::dot(light.m_Direction, light.m_Direction);
            const float angleFactor = dirLen2 > 1e-12f
                ? std::max(std::abs(light.m_Direction.y) / std::sqrt(dirLen2), 0.35f)
                : 1.0f;
            // Spot uses perspective — same constant/slope as ortho over-pushes contact shadows.
            const float typeScale = (light.m_Type == static_cast<int>(LightType::Spot)) ? 0.35f : 1.0f;
            ctx.SetDepthBias(
                light.m_ShadowBiasConstant * angleFactor * typeScale,
                light.m_ShadowBiasSlope * angleFactor * typeScale);

            for (auto entity : meshView) {
                auto& tc = meshView.get<TransformComponent>(entity);
                auto& mrc = meshView.get<MeshRendererComponent>(entity);
                if (!mrc.m_MeshAsset || !mrc.m_MeshAsset->IsLoaded())
                    continue;
                auto cpuMesh = mrc.m_MeshAsset->GetCPUMesh();
                if (!cpuMesh)
                    continue;

                const glm::mat4 model = tc.GetTransform();
                shadowShader->SetParameter("model", model);
                shadowShader->SetParameter("viewProj", light.m_LightViewProj);
                ctx.BindShader(m_ShadowShader);

                Nova::Core::Renderer::RHI::RHI_DrawIndexedCommand cmd{};
                cmd.m_Mesh = cpuMesh;
                cmd.m_Topology = Nova::Core::Renderer::RHI::RHI_PrimitiveTopology::Triangles;
                cmd.m_IndexType = Nova::Core::Renderer::RHI::RHI_IndexType::UInt32;
                cmd.m_IndexCount = static_cast<uint32_t>(cpuMesh->GetIndices().size());
                ctx.DrawIndexed(cmd);
            }

            ctx.EndDepthLayer();
        }
    }

} // namespace Nova::App