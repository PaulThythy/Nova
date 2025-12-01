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

        std::shared_ptr<Renderer::Mesh> cpuPlane = Renderer::Mesh::CreatePlane();
        std::shared_ptr<Renderer::Mesh> glPlane = std::make_shared<Renderer::OpenGL::GL_Mesh>(*cpuPlane);

        entt::entity planeEntity = g_Scene.CreateEntity("Plane");

        auto& registry = g_Scene.GetRegistry();

        const glm::vec3 t{ 0.0f,0.0f,0.0f };
        const glm::vec3 r{ 0.0f,0.0f,0.0f };
        const glm::vec3 s{ 3.0f,3.0f,3.0f };

        registry.emplace<Scene::ECS::Components::TransformComponent>(planeEntity, t, r, s);
        registry.emplace<Scene::ECS::Components::MeshComponent>(planeEntity, glPlane);

        auto camera = std::make_shared<Renderer::Camera>(
            glm::vec3(10.0f, 10.0f, 0.0f),       // lookFrom
            glm::vec3(0.0f, 0.0f, 0.0f),        // lookAt
            glm::vec3(0.0f, 1.0f, 0.0f),        // up
            45.0f,                                    // FOV in degree
            16.0f / 9.0f,                             // aspect ratio
            0.1f,                                     // near
            100.0f,                                   // far
            true                                      // perspective
        );

        entt::entity cameraEntity = g_Scene.CreateEntity("Camera");

        g_Scene.SetMainCamera(cameraEntity);

        registry.emplace<Scene::ECS::Components::CameraComponent>(
            cameraEntity,
            camera,
            true // isPrimary
        );

        glPlane->Upload(*cpuPlane);

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
        ReleaseFramebuffer();

        if (m_SceneProgram) {
            glDeleteProgram(m_SceneProgram);
            m_SceneProgram = 0;
        }

        auto& registry = g_Scene.GetRegistry();
        registry.clear();

        if (g_AppLayer == this)
            g_AppLayer = nullptr;
    }

    void AppLayer::SetViewportSize(float width, float height) {
        if (width <= 0.0f || height <= 0.0f)
            return;

        if (m_ViewportSize.x == width && m_ViewportSize.y == height)
            return;

        m_ViewportSize = { width, height };
        InvalidateFramebuffer();
    }

    void AppLayer::InvalidateFramebuffer() {
        ReleaseFramebuffer();

        if (m_ViewportSize.x <= 0.0f || m_ViewportSize.y <= 0.0f)
            return;

        glGenFramebuffers(1, &m_Framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);

        // Color attachment
        glGenTextures(1, &m_ColorAttachment);
        glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
            (GLsizei)m_ViewportSize.x, (GLsizei)m_ViewportSize.y,
            0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, m_ColorAttachment, 0);

        // Depth-stencil
        glGenRenderbuffers(1, &m_DepthAttachment);
        glBindRenderbuffer(GL_RENDERBUFFER, m_DepthAttachment);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
            (GLsizei)m_ViewportSize.x, (GLsizei)m_ViewportSize.y);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
            GL_RENDERBUFFER, m_DepthAttachment);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "AppLayer: Framebuffer is incomplete!" << std::endl;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void AppLayer::ReleaseFramebuffer() {
        if (m_DepthAttachment) {
            glDeleteRenderbuffers(1, &m_DepthAttachment);
            m_DepthAttachment = 0;
        }
        if (m_ColorAttachment) {
            glDeleteTextures(1, &m_ColorAttachment);
            m_ColorAttachment = 0;
        }
        if (m_Framebuffer) {
            glDeleteFramebuffers(1, &m_Framebuffer);
            m_Framebuffer = 0;
        }
    }

    void AppLayer::OnUpdate(float dt) {
        (void)dt;
        //later
    }

    void AppLayer::BeginScene() {
        if (m_ViewportResizePending) {
            m_ViewportResizePending = false;
            m_ViewportSize = m_PendingViewportSize;

            InvalidateFramebuffer();

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

        if (!m_Framebuffer || !m_SceneProgram)
            return;

        glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
        glViewport(0, 0,
            (GLsizei)m_ViewportSize.x,
            (GLsizei)m_ViewportSize.y);
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
        glFrontFace(GL_CCW);

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
        ImGui::DockSpace(
            dockspace_id,
            ImVec2(0.0f, 0.0f),
            ImGuiDockNodeFlags_PassthruCentralNode
        );

        SetupDockSpace(dockspace_id);

        ImGui::End();

        UI::Panels::HierarchyPanel::Render();
        UI::Panels::ViewportPanel::Render();
        UI::Panels::InspectorPanel::Render();
        UI::Panels::AssetBrowserPanel::Render();
    }

} // namespace Nova::App