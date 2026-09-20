#include "App/AppScene.h"

#include <array>
#include <cmath>
#include <random>
#include <string>

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

        auto cubeAsset = AssetManager::Get().Acquire<MeshAsset>("Engine://Primitives/Cube", MeshAssetDesc{ .m_AABBTreeDepth = 1 }).GetAssetRef();
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

        auto torusAsset = AssetManager::Get().Acquire<MeshAsset>("Engine://Primitives/Torus", MeshAssetDesc{ .m_AABBTreeDepth = 1 }).GetAssetRef();
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

        auto sphereAsset = AssetManager::Get().Acquire<MeshAsset>("Engine://Primitives/Sphere", MeshAssetDesc{ .m_AABBTreeDepth = 1 }).GetAssetRef();
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

        auto planeAsset = AssetManager::Get().Acquire<MeshAsset>("Engine://Primitives/Plane", MeshAssetDesc{ .m_AABBTreeDepth = 1 }).GetAssetRef();
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

    void AppScene::SetupStressTestScene(int objectCount) {
        if (objectCount < 0)
            objectCount = 0;

        m_Camera = std::make_shared<Camera>(
            glm::vec3(45.0f, 35.0f, 45.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            45.0f,
            16.0f / 9.0f,
            0.1f,
            500.0f,
            true
        );
        m_Camera->m_IsPerspective = true;
        m_Camera->m_FOV = 45.0f;
        m_Camera->m_NearPlane = 0.1f;
        m_Camera->m_FarPlane = 500.0f;
        m_Camera->m_Up = { 0.0f, 1.0f, 0.0f };

        entt::entity cameraEntity = m_Scene.CreateEntity("Camera");
        m_Scene.SetMainCamera(cameraEntity);

        auto& registry = m_Scene.GetRegistry();
        registry.emplace<CameraComponent>(cameraEntity, m_Camera, true);

        {
            entt::entity dirLightEntity = m_Scene.CreateEntity("DirectionalLight");
            registry.emplace<TransformComponent>(dirLightEntity,
                glm::vec3(0.0f, 20.0f, 0.0f),
                glm::vec3(glm::radians(-50.0f), glm::radians(35.0f), 0.0f),
                glm::vec3(1.0f));
            auto dirLight = std::make_shared<Light>();
            dirLight->m_Type = LightType::Directional;
            dirLight->m_Color = glm::vec3(1.0f);
            dirLight->m_Intensity = 3.0f;
            dirLight->m_Direction = glm::normalize(glm::vec3(-0.4f, -1.0f, -0.25f));
            dirLight->m_LightShadow = true;
            dirLight->m_ShadowBiasConstant = 0.5f;
            dirLight->m_ShadowBiasSlope = 1.0f;
            dirLight->m_ShadowNormalBias = 0.012f;
            registry.emplace<LightComponent>(dirLightEntity, dirLight);
        }

        auto planeAsset = AssetManager::Get().Acquire<MeshAsset>("Engine://Primitives/Plane", MeshAssetDesc{ .m_AABBTreeDepth = 1 }).GetAssetRef();
        planeAsset->Load();
        entt::entity planeEntity = m_Scene.CreateEntity("Ground");
        registry.emplace<TransformComponent>(planeEntity,
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(100.0f, 1.0f, 100.0f));
        {
            Nova::Core::Renderer::RHI::Material mat{};
            mat.m_BaseColor = glm::vec3(0.35f, 0.37f, 0.40f);
            registry.emplace<MeshRendererComponent>(planeEntity, planeAsset, mat);
            registry.emplace<MeshComponent>(planeEntity, planeAsset);
        }

        struct PrimitiveInfo {
            const char* m_AssetPath;
            const char* m_NamePrefix;
            float m_HalfHeight; // distance from local origin to bottom at scale 1
        };

        // Defaults match RHI_Mesh factories (cube/sphere/cyl halfExtent|radius 0.5, torus minor 0.2,
        // capsule height 1 + two hemispheres of radius 0.5 → half-height 1).
        static constexpr std::array<PrimitiveInfo, 5> kPrimitives{{
            { "Engine://Primitives/Cube",     "Cube",     0.5f },
            { "Engine://Primitives/Sphere",   "Sphere",   0.5f },
            { "Engine://Primitives/Torus",    "Torus",    0.2f },
            { "Engine://Primitives/Cylinder", "Cylinder", 0.5f },
            { "Engine://Primitives/Capsule",  "Capsule",  1.0f },
        }};

        std::array<std::shared_ptr<MeshAsset>, kPrimitives.size()> assets{};
        for (size_t i = 0; i < kPrimitives.size(); ++i) {
            assets[i] = AssetManager::Get().Acquire<MeshAsset>(kPrimitives[i].m_AssetPath, MeshAssetDesc{ .m_AABBTreeDepth = 1 }).GetAssetRef();
            assets[i]->Load();
        }

        std::mt19937 rng(42u);
        std::uniform_int_distribution<int> typeDist(0, static_cast<int>(kPrimitives.size()) - 1);
        std::uniform_real_distribution<float> posDist(-40.0f, 40.0f);
        std::uniform_real_distribution<float> yawDist(0.0f, glm::two_pi<float>());
        std::uniform_real_distribution<float> scaleDist(0.4f, 1.8f);
        std::uniform_real_distribution<float> colorDist(0.15f, 1.0f);

        for (int i = 0; i < objectCount; ++i) {
            const int type = typeDist(rng);
            const auto& info = kPrimitives[static_cast<size_t>(type)];
            const float scale = scaleDist(rng);
            const float y = info.m_HalfHeight * scale;

            entt::entity entity = m_Scene.CreateEntity(std::string(info.m_NamePrefix) + "_" + std::to_string(i));
            registry.emplace<TransformComponent>(entity,
                glm::vec3(posDist(rng), y, posDist(rng)),
                glm::vec3(0.0f, yawDist(rng), 0.0f),
                glm::vec3(scale));

            Nova::Core::Renderer::RHI::Material mat{};
            mat.m_BaseColor = glm::vec3(colorDist(rng), colorDist(rng), colorDist(rng));
            registry.emplace<MeshRendererComponent>(entity, assets[static_cast<size_t>(type)], mat);
            registry.emplace<MeshComponent>(entity, assets[static_cast<size_t>(type)]);
        }
    }

} // namespace Nova::App