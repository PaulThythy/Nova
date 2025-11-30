#include "App/EditorLayer.h"
#include "App/GameLayer.h"

#include "Scene/ECS/Components/CameraComponent.h"

#include "imgui.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Nova::App {

    using Nova::Core::Scene::ECS::Components::CameraComponent;
    using Nova::Core::Scene::ECS::Components::TransformComponent;
    using Nova::Core::Scene::ECS::Components::MeshComponent;

    EditorLayer::~EditorLayer() = default;

    void EditorLayer::OnEvent(Event& e) {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<KeyReleasedEvent>(
            [this](KeyReleasedEvent& ev) { return OnKeyReleased(ev); });
    }

    void EditorLayer::OnAttach() {
        std::string root = NOVA_APP_ROOT_DIR;
        m_SceneProgram = Nova::Core::Renderer::OpenGL::LoadRenderShader(
            root + "/shaders/OpenGL/scene/scene.vert",
            root + "/shaders/OpenGL/scene/scene.frag"
        );

        if (!m_SceneProgram) {
            std::cerr << "Failed to load scene shader program\n";
        }

        const float axisLength = 2.0f;

        // X axis : red
        m_XAxis = std::make_unique<Line>(
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(axisLength, 0.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f)
        );

        // Y axis : green
        m_YAxis = std::make_unique<Line>(
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, axisLength),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        // Z axis : blue
        m_ZAxis = std::make_unique<Line>(
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, axisLength, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );

        m_Grid = std::make_unique<Grid>(
            10,
            1.0f,
            glm::vec3(0.3f)
        );
    }

    void EditorLayer::OnDetach() {
        ReleaseFramebuffer();

        if (m_SceneProgram) {
            glDeleteProgram(m_SceneProgram);
            m_SceneProgram = 0;
        }
    }

    void EditorLayer::SetViewportSize(float width, float height) {
        if (width <= 0.0f || height <= 0.0f)
            return;

        if (m_ViewportSize.x == width && m_ViewportSize.y == height)
            return;

        m_ViewportSize = { width, height };
        InvalidateFramebuffer();
    }

    void EditorLayer::InvalidateFramebuffer() {
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

        // Depth-stencil attachment
        glGenRenderbuffers(1, &m_DepthAttachment);
        glBindRenderbuffer(GL_RENDERBUFFER, m_DepthAttachment);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
            (GLsizei)m_ViewportSize.x, (GLsizei)m_ViewportSize.y);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
            GL_RENDERBUFFER, m_DepthAttachment);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "EditorLayer: Framebuffer is incomplete!" << std::endl;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void EditorLayer::ReleaseFramebuffer() {
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

    void EditorLayer::OnUpdate(float dt) {
        (void)dt;
        //later
    }

    void EditorLayer::OnRender() {
        auto& registry = Nova::App::g_Scene.GetRegistry();

        entt::entity activeCam = entt::null;
        auto camView = registry.view<CameraComponent>();

        Nova::Core::Renderer::Camera* cameraPtr = nullptr;

        for (auto entity : camView) {
            auto& camComp = camView.get<CameraComponent>(entity);
            if (camComp.m_Camera && camComp.m_IsPrimary) {
                cameraPtr = camComp.m_Camera.get();
                break;
            }
        }

        if (!cameraPtr) {
            return;
        }

        if (m_Framebuffer && m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f) {
            glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
            glViewport(0, 0,
                (GLsizei)m_ViewportSize.x,
                (GLsizei)m_ViewportSize.y);
            glEnable(GL_DEPTH_TEST);
            glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        glm::mat4 view       = cameraPtr->GetViewMatrix();
        glm::mat4 projection = cameraPtr->GetProjectionMatrix();
        glm::mat4 viewProj   = projection * view;

        if (m_SceneProgram) {
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
                auto& meshComp  = viewMeshes.get<MeshComponent>(entity);

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

        glm::mat4 mvp = viewProj;

        if (m_Grid) {
            m_Grid->m_MVP = mvp;
            m_Grid->draw();
        }
        if (m_XAxis) {
            m_XAxis->m_MVP = mvp;
            m_XAxis->draw();
        }
        if (m_YAxis) {
            m_YAxis->m_MVP = mvp;
            m_YAxis->draw();
        }
        if (m_ZAxis) {
            m_ZAxis->m_MVP = mvp;
            m_ZAxis->draw();
        }

        if (m_Framebuffer) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }

    void EditorLayer::OnImGuiRender() {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Viewport",
            nullptr,
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse);

        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        SetViewportSize(viewportPanelSize.x, viewportPanelSize.y);

        if (m_ColorAttachment != 0) {
            ImGui::Image(
                (ImTextureID)(uintptr_t)m_ColorAttachment,
                viewportPanelSize,
                ImVec2(0.0f, 1.0f),
                ImVec2(1.0f, 0.0f)
            );
        }
        else {
            ImGui::Text("Framebuffer not ready.");
            ImGui::Text("Viewport size: %.0f x %.0f", viewportPanelSize.x, viewportPanelSize.y);
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    bool EditorLayer::OnKeyReleased(KeyReleasedEvent& e) {
        if (e.GetKeyCode() == SDLK_SPACE) {
            Nova::Core::Application::Get().GetLayerStack().QueueLayerTransition<GameLayer>(this);
            std::cout << "EditorLayer: Transition to GameLayer requested." << std::endl;
            return true;
        }
        return false;
    }

} // namespace Nova::App