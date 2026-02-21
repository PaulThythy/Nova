#include "App/AppLayer.h"

#include "Scene/ECS/Components/TransformComponent.h"
#include "Scene/ECS/Components/MeshComponent.h"
#include "Scene/ECS/Components/CameraComponent.h"

#include "Asset/AssetManager.h"
#include "Asset/Assets/MeshAsset.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <filesystem>

#include "App/GameLayer.h"
#include "App/EditorLayer.h"

namespace Nova::App {

    using namespace Nova::Core;

    Nova::Core::Scene::Scene g_Scene("Scene_test");
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
        ImGui::DockBuilderSplitNode(
            dockspace_id,
            ImGuiDir_Down,
            0.19f,
            &dock_down,
            &dock_center
        );

        ImGuiID dock_right = 0;
        ImGuiID dock_left = 0;
        ImGui::DockBuilderSplitNode(
            dock_center,
            ImGuiDir_Right,
            0.27f,
            &dock_right,
            &dock_left
        );

        ImGuiID dock_right_top = 0;
        ImGuiID dock_right_bottom = 0;
        ImGui::DockBuilderSplitNode(
            dock_right,
            ImGuiDir_Up,
            0.50f,
            &dock_right_top,
            &dock_right_bottom
        );

        // Dock windows
        ImGui::DockBuilderDockWindow(g_Scene.GetName().c_str(), dock_left);
        ImGui::DockBuilderDockWindow("Hierarchy", dock_right_top);
        ImGui::DockBuilderDockWindow("Inspector", dock_right_bottom);
        ImGui::DockBuilderDockWindow("Asset Browser", dock_down);

        ImGui::DockBuilderFinish(dockspace_id);
        s_DockInitialized = true;
    }

    void AppLayer::OnEvent(Event& e) {
        EventDispatcher dispatcher(e);

        dispatcher.Dispatch<ImGuiPanelResizeEvent>(
            [this](ImGuiPanelResizeEvent& ev) { return OnImGuiPanelResize(ev); }
        );
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

    bool AppLayer::OnImGuiPanelResize(ImGuiPanelResizeEvent& e) {
        if (e.GetPanelName() != "Viewport")
            return false;

        float width = e.GetWidth();
        float height = e.GetHeight();

        if (width <= 0.0f || height <= 0.0f)
            return false;

        m_PendingViewportSize = { width, height };
        m_ViewportResizePending = true;

        return false;
    }

    void AppLayer::OnAttach() {
        g_AppLayer = this;

        Nova::Core::GraphicsAPI graphicsAPI = Nova::Core::Application::Get().GetWindow().GetGraphicsAPI();

        Nova::Core::Application::Get().GetImGuiLayer().SetImGuiBackend(graphicsAPI);

        auto& registry = g_Scene.GetRegistry();

        using Nova::Core::Asset::AssetManager;
        using Nova::Core::Asset::Assets::MeshAsset;
        using Nova::Core::Asset::Assets::MeshAssetDesc;

        auto planeAsset = AssetManager::Get().Acquire<MeshAsset>("Engine://Primitives/Plane").GetAssetRef();

        MeshAssetDesc cubeDesc{};
        cubeDesc.m_HalfExtent = 0.5f;
        auto cubeAsset = AssetManager::Get().Acquire<MeshAsset>("Engine://Primitives/Cube", cubeDesc).GetAssetRef();

        auto sphereAsset = AssetManager::Get().Acquire<MeshAsset>("Engine://Primitives/Sphere").GetAssetRef();

        if (planeAsset) planeAsset->Load();
        if (cubeAsset) cubeAsset->Load();
        if (sphereAsset) sphereAsset->Load();

        entt::entity planeEntity = g_Scene.CreateEntity("Plane");
        entt::entity cubeEntity = g_Scene.CreateEntity("Cube");
        entt::entity sphere1Entity = g_Scene.CreateEntity("Sphere1");
        entt::entity sphere2Entity = g_Scene.CreateEntity("Sphere2");
        entt::entity sphere3Entity = g_Scene.CreateEntity("Sphere3");

        g_Scene.ParentEntity(sphere2Entity, sphere1Entity);
        g_Scene.ParentEntity(sphere3Entity, sphere1Entity);

        const glm::vec3 t{ 0.0f,0.0f,0.0f };
        const glm::vec3 r{ 0.0f,0.0f,0.0f };
        const glm::vec3 s{ 3.0f,3.0f,3.0f };

        registry.emplace<Scene::ECS::Components::TransformComponent>(planeEntity, t, r, s);
        registry.emplace<Scene::ECS::Components::MeshComponent>(planeEntity, planeAsset);

        registry.emplace<Scene::ECS::Components::TransformComponent>(cubeEntity,
            glm::vec3(0.0f, 0.5f, 0.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 1.0f)
        );
        registry.emplace<Scene::ECS::Components::MeshComponent>(cubeEntity, cubeAsset);

        registry.emplace<Scene::ECS::Components::TransformComponent>(sphere1Entity,
            glm::vec3(2.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 1.0f)
        );
        registry.emplace<Scene::ECS::Components::MeshComponent>(sphere1Entity, sphereAsset);

        registry.emplace<Scene::ECS::Components::TransformComponent>(sphere2Entity,
            glm::vec3(3.0f, 0.7f, 0.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.7f, 0.7f, 0.7f)
        );
        registry.emplace<Scene::ECS::Components::MeshComponent>(sphere2Entity, sphereAsset);

        registry.emplace<Scene::ECS::Components::TransformComponent>(sphere3Entity,
            glm::vec3(3.0f, 0.5f, 0.5f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.5f, 0.5f, 0.5f)
        );
        registry.emplace<Scene::ECS::Components::MeshComponent>(sphere3Entity, sphereAsset);

        auto camera = std::make_shared<Renderer::Graphics::Camera>(
            glm::vec3(2.5f, 2.5f, 5.0f),               // lookFrom
            glm::vec3(0.0f, 0.0f, 0.0f),                // lookAt
            glm::vec3(0.0f, 1.0f, 0.0f),                // up
            45.0f,                                      // FOV in degree
            16.0f / 9.0f,                               // aspect ratio
            0.1f,                                       // near
            100.0f,                                     // far
            true                                        // perspective
        );

        entt::entity cameraEntity = g_Scene.CreateEntity("Camera");

        g_Scene.SetMainCamera(cameraEntity);

        registry.emplace<Scene::ECS::Components::CameraComponent>(
            cameraEntity,
            camera,
            true // isPrimary
        );

        std::filesystem::path p = std::filesystem::current_path();
        std::filesystem::path shaderDir = p / "Nova-App" / "Resources" / "Editor" / "Shaders";

        std::filesystem::path vertShaderDir = shaderDir / "Scene" / "scene.vert";
        std::filesystem::path fragShaderDir = shaderDir / "Scene" / "scene.frag";

        std::string vertShaderDirStr = vertShaderDir.string();
        std::string fragShaderDirStr = fragShaderDir.string();

        m_SceneProgram = Nova::Core::Renderer::Backends::OpenGL::LoadRenderShader(vertShaderDirStr, fragShaderDirStr);

        if (!m_SceneProgram) {
            std::cerr << "Failed to load scene shader program\n";
        }
    }

    void AppLayer::OnDetach() {
        m_FrameBuffer.reset();

        if (m_SceneProgram) {
            glDeleteProgram(m_SceneProgram);
            m_SceneProgram = 0;
        }

        g_Scene.Clear();

        if (g_AppLayer == this)
            g_AppLayer = nullptr;
    }

    void AppLayer::OnUpdate(float dt) {
        (void)dt;
        //later
    }

    void AppLayer::BeginScene() {
        // Handle viewport resizing
        if (m_ViewportResizePending) {
            m_ViewportResizePending = false;
            m_ViewportSize = m_PendingViewportSize;
            
            // Recreate framebuffer if it doesn't exist
            if (!m_FrameBuffer) {
                m_FrameBuffer = std::make_unique<Renderer::Backends::OpenGL::GL_FrameBuffer>();
            }
            
            // Resize framebuffer
            m_FrameBuffer->Resize(
                static_cast<int>(m_ViewportSize.x),
                static_cast<int>(m_ViewportSize.y)
            );
            
            // Update aspect ratio of primary camera
            auto& registry = g_Scene.GetRegistry();
            using Nova::Core::Scene::ECS::Components::CameraComponent;

            auto camView = registry.view<CameraComponent>();
            for (auto entity : camView) {
                auto& camComp = camView.get<CameraComponent>(entity);
                if (camComp.m_Camera && camComp.m_IsPrimary) {
                    camComp.m_Camera->m_AspectRatio = m_ViewportSize.x / m_ViewportSize.y;
                    break;
                }
            }
        }

        if (!m_FrameBuffer || !m_SceneProgram)
            return;

        // Render to framebuffer
        m_FrameBuffer->Bind();
        //TODO remove all gl functions from AppLayer
        glViewport(0, 0, static_cast<GLsizei>(m_ViewportSize.x), static_cast<GLsizei>(m_ViewportSize.y));
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void AppLayer::RenderScene(const glm::mat4& viewProj) {
        // Render all meshes in the scene
        auto& registry = g_Scene.GetRegistry();

        using Nova::Core::Scene::ECS::Components::CameraComponent;
        using Nova::Core::Scene::ECS::Components::TransformComponent;
        using Nova::Core::Scene::ECS::Components::MeshComponent;

        if (!m_SceneProgram)
            return;

        glUseProgram(m_SceneProgram);

        GLint locVP = glGetUniformLocation(m_SceneProgram, "u_ViewProjection");
        if (locVP != -1) {
            glUniformMatrix4fv(locVP, 1, GL_FALSE, glm::value_ptr(viewProj));
        }

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CW);

        auto viewMeshes = registry.view<TransformComponent, MeshComponent>();

        viewMeshes.each([&](entt::entity entity, TransformComponent& transform, MeshComponent& meshComp) {
            if (!meshComp.m_MeshAsset)
                return;

            auto renderMesh = meshComp.m_MeshAsset->GetGPUMesh();
            if (!renderMesh)
                return;

            glm::mat4 model = transform.GetTransform();

            GLint locModel = glGetUniformLocation(m_SceneProgram, "u_Model");
            if (locModel != -1) {
                glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
            }

            renderMesh->Bind();
            renderMesh->Draw();
            renderMesh->Unbind();
        });

        glDisable(GL_CULL_FACE);
    }

    void AppLayer::EndScene() {
        // Unbind framebuffer
        // if we don't do this verification, the first message is "No framebuffer to unbind", and program crashes
        if (m_FrameBuffer) {
            //std::cout << "Unbinding framebuffer if it exists" << std::endl;
            m_FrameBuffer->Unbind();
        }
        else {
            //std::cout << "No framebuffer to unbind" << std::endl;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }

    void AppLayer::OnRender() {/*empty*/ }

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
        UI::Panels::ScenePanel::Render(g_Scene.GetName());
        UI::Panels::HierarchyPanel::Render();
        UI::Panels::InspectorPanel::Render();
        UI::Panels::AssetBrowserPanel::Render();
    }

} // namespace Nova::App
