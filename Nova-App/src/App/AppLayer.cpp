#include "App/AppLayer.h"

#include "Scene/ECS/Components/TransformComponent.h"
#include "Scene/ECS/Components/MeshComponent.h"
#include "Scene/ECS/Components/CameraComponent.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

namespace Nova::App {

    using namespace Nova::Core;

    Nova::Core::Scene::Scene g_Scene;
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
        ImGuiID dock_main = 0;
        ImGui::DockBuilderSplitNode(
            dockspace_id,
            ImGuiDir_Down,
            0.19f,
            &dock_down,
            &dock_main
        );

        ImGuiID dock_right = 0;
        ImGuiID dock_center = 0;
        ImGui::DockBuilderSplitNode(
            dock_main,
            ImGuiDir_Right,
            0.25f,
            &dock_right,
            &dock_center
        );

        ImGuiID dock_right_top = 0;
        ImGuiID dock_right_bottom = 0;
        ImGui::DockBuilderSplitNode(
            dock_right,
            ImGuiDir_Up,
            0.5f,
            &dock_right_top,
            &dock_right_bottom
        );

        ImGui::DockBuilderDockWindow("Hierarchy", dock_right_top);
        ImGui::DockBuilderDockWindow("Inspector", dock_right_bottom);
        ImGui::DockBuilderDockWindow("Viewport", dock_center);
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

        auto& registry = g_Scene.GetRegistry();

        std::shared_ptr<Renderer::Mesh> cpuPlane = Renderer::Mesh::CreatePlane();
        std::shared_ptr<Renderer::Mesh> glPlane = std::make_shared<Renderer::OpenGL::GL_Mesh>(*cpuPlane);

        std::shared_ptr<Renderer::Mesh> cpuCube = Renderer::Mesh::CreateCube();
        std::shared_ptr<Renderer::Mesh> glCube = std::make_shared<Renderer::OpenGL::GL_Mesh>(*cpuCube);

        std::shared_ptr<Renderer::Mesh> cpuSphere = Renderer::Mesh::CreateSphere(1.0f, 3, 6);
        std::shared_ptr<Renderer::Mesh> glSphere = std::make_shared<Renderer::OpenGL::GL_Mesh>(*cpuSphere);

        entt::entity planeEntity = g_Scene.CreateEntity("Plane");
        entt::entity cubeEntity = g_Scene.CreateEntity("Cube");
        entt::entity sphereEntity = g_Scene.CreateEntity("Sphere");

        const glm::vec3 t{ 0.0f,0.0f,0.0f };
        const glm::vec3 r{ 0.0f,0.0f,0.0f };
        const glm::vec3 s{ 10.0f,10.0f,10.0f };

        registry.emplace<Scene::ECS::Components::TransformComponent>(planeEntity, t, r, s);
        registry.emplace<Scene::ECS::Components::MeshComponent>(planeEntity, glPlane);

        registry.emplace<Scene::ECS::Components::TransformComponent>(cubeEntity,
            glm::vec3(0.0f, 0.5f, 0.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 1.0f)
        );
        registry.emplace<Scene::ECS::Components::MeshComponent>(cubeEntity, glCube);

        registry.emplace<Scene::ECS::Components::TransformComponent>(sphereEntity,
            glm::vec3(2.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 1.0f)
        );
        registry.emplace<Scene::ECS::Components::MeshComponent>(sphereEntity, glSphere);

        auto camera = std::make_shared<Renderer::Camera>(
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

        glPlane->Upload(*cpuPlane);
        glCube->Upload(*cpuCube);
        glSphere->Upload(*cpuSphere);

        std::string root = NOVA_APP_ROOT_DIR;
        m_SceneProgram = Nova::Core::Renderer::OpenGL::LoadRenderShader(
            root + "/shaders/OpenGL/scene/scene.vert",
            root + "/shaders/OpenGL/scene/scene.frag"
        );

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

        auto& registry = g_Scene.GetRegistry();
        registry.clear();

        if (g_AppLayer == this)
            g_AppLayer = nullptr;
    }

    void AppLayer::OnUpdate(float dt) {
        (void)dt;
        //later
    }

    void AppLayer::BeginScene() {
        if (m_ViewportResizePending) {
            m_ViewportResizePending = false;
            m_ViewportSize = m_PendingViewportSize;

            if (!m_FrameBuffer) {
                m_FrameBuffer = std::make_unique<Renderer::OpenGL::GL_FrameBuffer>();
            }

            m_FrameBuffer->Resize(
                static_cast<int>(m_ViewportSize.x),
                static_cast<int>(m_ViewportSize.y)
            );

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

        m_FrameBuffer->Bind();
        glViewport(0, 0, static_cast<GLsizei>(m_ViewportSize.x), static_cast<GLsizei>(m_ViewportSize.y));
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void AppLayer::RenderScene(const glm::mat4& viewProj) {
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
        for (auto entity : viewMeshes) {
            auto& transform = viewMeshes.get<TransformComponent>(entity);
            auto& meshComp = viewMeshes.get<MeshComponent>(entity);

            if (!meshComp.m_Mesh)
                continue;

            glm::mat4 model = transform.GetTransform();

            GLint locModel = glGetUniformLocation(m_SceneProgram, "u_Model");
            if (locModel != -1) {
                glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
            }

            meshComp.m_Mesh->Bind();
            meshComp.m_Mesh->Draw();
            meshComp.m_Mesh->Unbind();
        }

        glDisable(GL_CULL_FACE);
    }

    void AppLayer::EndScene() {
        if (m_FrameBuffer)
            m_FrameBuffer->Unbind();
        else
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

        UI::Panels::HierarchyPanel::Render();
        UI::Panels::ViewportPanel::Render();
        UI::Panels::InspectorPanel::Render();
        UI::Panels::AssetBrowserPanel::Render();
    }

} // namespace Nova::App