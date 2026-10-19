#ifndef ASSIMPLAYER_H
#define ASSIMPLAYER_H

#include <cstdint>
#include <memory>

#include <glm/glm.hpp>

#include "Asset/Assets/MeshAsset.h"
#include "Core/Layer.h"
#include "Events/ApplicationEvents.h"
#include "Events/Event.h"
#include "Events/InputEvents.h"
#include "Math/Camera.h"
#include "Renderer/RHI/RHI_Renderer.h"
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
    bool OnKeyPressed(Nova::Core::Events::KeyPressedEvent& e);
    bool OnWindowResized(Nova::Core::Events::WindowResizeEvent& e);

    void SetupRenderer(uint32_t width, uint32_t height);
    void SetupScene();
    void PushFrameUniforms();
    void PushSceneUniforms();
    void RenderScene(Nova::Core::Renderer::RHI::IPassContext& ctx);
    void DrawMeshEntity(Nova::Core::Renderer::RHI::IPassContext& ctx, entt::entity entity);

    void DrawMeshInfoPanel(
        const char* name,
        const std::shared_ptr<Nova::Core::Asset::Assets::MeshAsset>& asset,
        bool loaded) const;

    std::unique_ptr<Nova::Core::Renderer::RHI::IRenderer> m_Renderer;
    Nova::Core::Renderer::RHI::RHI_ShaderHandle m_SimpleLitShader{};
    Nova::Core::Renderer::RHI::RHI_TextureHandle m_ShadowMaps{};

    Nova::Core::Scene::Scene m_Scene{ "AssimpObjectLoading" };
    std::shared_ptr<Nova::Core::Math::Camera> m_Camera;
    std::shared_ptr<Nova::Core::Asset::Assets::MeshAsset> m_SuzanneAsset;
    std::shared_ptr<Nova::Core::Asset::Assets::MeshAsset> m_CubeAsset;

    entt::entity m_SuzanneEntity{ entt::null };
    entt::entity m_CubeEntity{ entt::null };

    float m_ElapsedTime = 0.0f;
    float m_DeltaTime = 0.0f;
    uint32_t m_FrameIndex = 0;
    glm::vec2 m_ViewportSize{ 1280.0f, 720.0f };

    bool m_SuzanneLoaded = false;
    bool m_CubeLoaded = false;
};

#endif // ASSIMPLAYER_H