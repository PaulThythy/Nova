#ifndef ASSIMPLAYER_H
#define ASSIMPLAYER_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "Asset/AssetHandle.h"
#include "Asset/Assets/MeshAsset.h"
#include "Core/Layer.h"
#include "Core/UUID.h"
#include "Events/ApplicationEvents.h"
#include "Events/Event.h"
#include "Events/InputEvents.h"
#include "Math/Camera.h"
#include "Renderer/RHI/RHI_Renderer.h"
#include "Renderer/RHI/RHI_ShaderUniforms.h"
#include "Scene/Scene.h"

class AssimpLayer : public Nova::Core::Layer {
public:
    explicit AssimpLayer();
    ~AssimpLayer() override = default;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float dt) override;
    void OnBegin() override;
    void OnRender() override;
    void OnEnd() override;
    void OnImGuiRender() override;
    void OnEvent(Nova::Core::Events::Event& e) override;

private:
    struct SceneMesh {
        std::string m_Name;
        Nova::Core::Asset::AssetHandle<Nova::Core::Asset::Assets::MeshAsset> m_Asset;
        entt::entity m_Entity{ entt::null };
        bool m_Loaded = false;
        bool m_IsPrimitive = false;
        Nova::Core::UUID m_ExpectedAssetID = Nova::Core::kInvalidUUID;
        float m_SpinSign = 1.0f;
    };

    bool OnKeyPressed(Nova::Core::Events::KeyPressedEvent& e);
    bool OnWindowResized(Nova::Core::Events::WindowResizeEvent& e);

    void SetupRenderer(uint32_t width, uint32_t height);
    void SetupScene();
    void PushFrameUniforms();
    void PushSceneUniforms();
    void PrepareGpuInstances();
    void RenderScene(Nova::Core::Renderer::RHI::IPassContext& ctx);
    void DrawMeshEntity(Nova::Core::Renderer::RHI::IPassContext& ctx, entt::entity entity, uint32_t firstInstance);
    void DrawMeshInfoPanel(const SceneMesh& mesh) const;

    std::unique_ptr<Nova::Core::Renderer::RHI::IRenderer> m_Renderer;
    Nova::Core::Renderer::RHI::RHI_ShaderHandle m_SimpleLitShader{};

    Nova::Core::Scene::Scene m_Scene{ "AssimpObjectLoading" };
    std::shared_ptr<Nova::Core::Math::Camera> m_Camera;

    std::vector<SceneMesh> m_Primitives;
    SceneMesh m_Suzanne;
    SceneMesh m_Dragon;

    std::vector<Nova::Core::Renderer::RHI::InstanceGPU> m_Instances;
    std::vector<Nova::Core::Renderer::RHI::MaterialGPU> m_Materials;
    std::vector<entt::entity> m_DrawEntities;

    float m_ElapsedTime = 0.0f;
    float m_DeltaTime = 0.0f;
    uint32_t m_FrameIndex = 0;
    glm::vec2 m_ViewportSize{ 1280.0f, 720.0f };
};

#endif // ASSIMPLAYER_H