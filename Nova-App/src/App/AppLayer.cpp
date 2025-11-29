#include "App/AppLayer.h"

#include "imgui.h"

#include "Scene/ECS/Components/TransformComponent.h"
#include "Scene/ECS/Components/MeshComponent.h"
#include "Scene/ECS/Components/CameraComponent.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

namespace Nova::App {

    using namespace Nova::Core;

    Nova::Core::Scene::Scene g_Scene;

    AppLayer::~AppLayer() = default;

    void AppLayer::OnEvent(Event& e) {}

    void AppLayer::OnAttach() {
        std::shared_ptr<Renderer::Mesh> cpuPlane = Renderer::Mesh::CreatePlane();
        std::shared_ptr<Renderer::Mesh> glPlane = std::make_shared<Renderer::OpenGL::GL_Mesh>(*cpuPlane);

        entt::entity planeEntity = g_Scene.CreateEntity("Plane");

        auto& registry = g_Scene.GetRegistry();

        registry.emplace<Scene::ECS::Components::TransformComponent>(planeEntity);
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
    }

    void AppLayer::OnDetach() {
        auto& registry = g_Scene.GetRegistry();
        registry.clear();
    }

    void AppLayer::OnUpdate(float dt) {
        (void)dt;
        //later
    }

    void AppLayer::OnRender() {}

    void AppLayer::OnImGuiRender() {
        ImGui::Begin("Scene / Camera");

        auto& registry = g_Scene.GetRegistry();
        using CameraComp = Scene::ECS::Components::CameraComponent;

        entt::entity activeCam = entt::null;
        auto camView = registry.view<CameraComp>();

        for (auto entity : camView) {
            auto& camComp = camView.get<CameraComp>(entity);
            if (camComp.m_IsPrimary && camComp.m_Camera) {
                activeCam = entity;
                break;
            }
        }

        if (activeCam == entt::null && !camView.empty()) {
            activeCam = *camView.begin();
        }

        if (activeCam != entt::null) {
            auto& camComp = camView.get<CameraComp>(activeCam);

            if (camComp.m_Camera) {
                auto& cam = *camComp.m_Camera;

                ImGui::Separator();
                ImGui::Text("Active Camera");

                // Position
                ImGui::DragFloat3("Position", &cam.m_LookFrom.x, 0.1f);

                // Direction (rotation) : on édite le vecteur direction LookAt - LookFrom
                glm::vec3 dir = cam.m_LookAt - cam.m_LookFrom;
                if (ImGui::DragFloat3("Direction", &dir.x, 0.1f)) {
                    if (glm::length(dir) > 0.0001f) {
                        dir = glm::normalize(dir);
                        cam.m_LookAt = cam.m_LookFrom + dir;
                    }
                }

                // FOV / clipping / aspect
                ImGui::DragFloat("FOV", &cam.m_FOV, 0.1f, 1.0f, 179.0f);
                ImGui::DragFloat("Near Plane", &cam.m_NearPlane, 0.01f, 0.01f, cam.m_FarPlane - 0.1f);
                ImGui::DragFloat("Far Plane", &cam.m_FarPlane, 1.0f, cam.m_NearPlane + 0.1f, 10000.0f);
                ImGui::DragFloat("Aspect Ratio", &cam.m_AspectRatio, 0.01f, 0.1f, 10.0f);

                // Perspective / Ortho
                ImGui::Checkbox("Perspective", &cam.m_IsPerspective);
            }
        } else {
            ImGui::Separator();
            ImGui::Text("No active camera in scene.");
        }

        ImGuiIO& io = ImGui::GetIO();
        float fps = io.Framerate;
        float msPerFrame = fps > 0.0f ? 1000.0f / fps : 0.0f;

        ImGui::Separator();
        ImGui::Text("Frame time: %.3f ms (%.1f FPS)", msPerFrame, fps);

        ImGui::End();
    }

} // namespace Nova::App