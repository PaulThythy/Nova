#include "App/AppScene.h"

#include <glm/glm.hpp>

#include "Asset/AssetManager.h"
#include "Asset/Assets/MeshAsset.h"
#include "ECS/Components/CameraComponent.h"
#include "ECS/Components/LightComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/MeshRendererComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "Math/Light.h"
#include "Renderer/RHI/RHI_ShaderUniforms.h"

namespace Nova::App {

    using namespace Nova::Core::Asset;
    using namespace Nova::Core::Asset::Assets;
    using namespace Nova::Core::ECS::Components;
    using namespace Nova::Core::Math;

    AppScene::AppScene(const std::string& name) : m_Scene(name) {}

    void AppScene::Clear() {
        m_Scene.Clear();
        m_Camera.reset();
    }

    void AppScene::SetupDefaultScene() {
        m_Camera = std::make_shared<Camera>(
            glm::vec3(5.0f, 5.0f, 5.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            45.0f,
            16.0f / 9.0f,
            0.1f,
            100.0f,
            true
        );
        m_Camera->m_IsPerspective = true;
        m_Camera->m_FOV = 45.0f;
        m_Camera->m_NearPlane = 0.1f;
        m_Camera->m_FarPlane = 1000.0f;
        m_Camera->m_Up = { 0.0f, 1.0f, 0.0f };

        entt::entity cameraEntity = m_Scene.CreateEntity("Camera");
        m_Scene.SetMainCamera(cameraEntity);

        auto& registry = m_Scene.GetRegistry();
        registry.emplace<CameraComponent>(cameraEntity, m_Camera, true);

        auto cubeAsset = AssetManager::Get().Acquire<MeshAsset>(
            "Engine://Primitives/Cube", MeshAssetDesc{ .m_AABBTreeDepth = 1 }).GetAssetRef();
        cubeAsset->Load();
        entt::entity cubeEntity = m_Scene.CreateEntity("Cube");
        registry.emplace<TransformComponent>(cubeEntity,
            glm::vec3(0.0f, 0.5f, 0.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 1.0f));
        {
            Nova::Core::Renderer::RHI::Material mat{};
            mat.m_BaseColor = glm::vec3(0.0f, 1.0f, 0.0f);
            registry.emplace<MeshRendererComponent>(cubeEntity, cubeAsset, mat);
            registry.emplace<MeshComponent>(cubeEntity, cubeAsset);
        }

        auto torusAsset = AssetManager::Get().Acquire<MeshAsset>(
            "Engine://Primitives/Torus", MeshAssetDesc{ .m_AABBTreeDepth = 1 }).GetAssetRef();
        torusAsset->Load();
        entt::entity torusEntity = m_Scene.CreateEntity("Torus");
        registry.emplace<TransformComponent>(torusEntity,
            glm::vec3(2.0f, 0.25f, 1.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 1.0f));
        {
            Nova::Core::Renderer::RHI::Material mat{};
            mat.m_BaseColor = glm::vec3(1.0f, 0.5f, 0.0f);
            registry.emplace<MeshRendererComponent>(torusEntity, torusAsset, mat);
            registry.emplace<MeshComponent>(torusEntity, torusAsset);
        }

        auto sphereAsset = AssetManager::Get().Acquire<MeshAsset>(
            "Engine://Primitives/Sphere", MeshAssetDesc{ .m_AABBTreeDepth = 1 }).GetAssetRef();
        sphereAsset->Load();
        entt::entity sphereEntity = m_Scene.CreateEntity("Sphere");
        registry.emplace<TransformComponent>(sphereEntity,
            glm::vec3(0.0f, 0.5f, -1.5f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 1.0f));
        {
            Nova::Core::Renderer::RHI::Material mat{};
            mat.m_BaseColor = glm::vec3(0.0f, 0.0f, 1.0f);
            registry.emplace<MeshRendererComponent>(sphereEntity, sphereAsset, mat);
            registry.emplace<MeshComponent>(sphereEntity, sphereAsset);
        }

        auto planeAsset = AssetManager::Get().Acquire<MeshAsset>(
            "Engine://Primitives/Plane", MeshAssetDesc{ .m_AABBTreeDepth = 1 }).GetAssetRef();
        planeAsset->Load();
        entt::entity planeEntity = m_Scene.CreateEntity("Plane");
        registry.emplace<TransformComponent>(planeEntity,
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(10.0f, 10.0f, 10.0f));
        {
            Nova::Core::Renderer::RHI::Material mat{};
            registry.emplace<MeshRendererComponent>(planeEntity, planeAsset, mat);
            registry.emplace<MeshComponent>(planeEntity, planeAsset);
        }

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

        {
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
            spot->m_ShadowBiasConstant = 0.2f;
            spot->m_ShadowBiasSlope = 0.4f;
            spot->m_ShadowNormalBias = 0.003f;
            registry.emplace<LightComponent>(spotEntity, spot);
        }
    }

} // namespace Nova::App