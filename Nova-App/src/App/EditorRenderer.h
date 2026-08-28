#ifndef EDITORRENDERER_H
#define EDITORRENDERER_H

#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "App/EditorSelection.h"
#include "App/RenderDebugMode.h"
#include "Core/GraphicsAPI.h"
#include "Math/Camera.h"
#include "Renderer/RHI/RHI_Renderer.h"
#include "Renderer/RHI/RHI_RenderGraph.h"
#include "Renderer/RHI/RHI_ShaderUniforms.h"
#include "Scene/Scene.h"

namespace Nova::App {

    class EditorRenderer {
    public:
        void Initialize(Nova::Core::GraphicsAPI api, uint32_t width, uint32_t height);
        void Shutdown();

        Nova::Core::Renderer::RHI::IRenderer* GetRenderer() const { return m_Renderer.get(); }

        void BeginFrame();
        void RenderFrame();
        void EndFrame();
        void Resize(int width, int height);

        /** Bind scene/camera/selection for the current frame (call before UploadLights / PushGlobals). */
        void BindFrame(Nova::Core::Scene::Scene& scene, Nova::Core::Math::Camera& camera, EditorSelection& selection);

        void UploadLights();
        void PushGlobals(float elapsedTime, float deltaTime, uint32_t& frameIndex, const glm::vec2& viewportSize);
        void RebindAfterResize();

        Nova::Core::Renderer::RHI::RHI_TextureHandle GetSceneColor() const { return m_SceneColor; }
        Nova::Core::Renderer::RHI::RHI_TextureHandle GetSceneDepth() const { return m_SceneDepth; }
        Nova::Core::Renderer::RHI::RHI_TextureHandle GetShadowMaps() const { return m_ShadowMaps; }

        void SetShowGrid(bool show) { m_ShowGrid = show; }
        bool IsGridVisible() const { return m_ShowGrid; }

        void SetShowAABB(bool show) { m_ShowAABB = show; }
        bool IsAABBVisible() const { return m_ShowAABB; }

        void SetRenderDebugMode(RenderDebugMode mode) { m_RenderDebugMode = mode; }
        RenderDebugMode GetRenderDebugMode() const { return m_RenderDebugMode; }

    private:
        Nova::Core::Renderer::RHI::RHI_ShaderHandle GetActiveSceneShader() const;

        void RenderScene(Nova::Core::Renderer::RHI::IPassContext& ctx);
        void RenderSelectionMask(Nova::Core::Renderer::RHI::IPassContext& ctx);
        void RenderSelectionBlur(Nova::Core::Renderer::RHI::IPassContext& ctx, Nova::Core::Renderer::RHI::RHI_ShaderHandle blurShader);
        void RenderSelectionComposite(Nova::Core::Renderer::RHI::IPassContext& ctx);
        void BindSelectionOutlineTextures();
        void RenderAABBs(Nova::Core::Renderer::RHI::IPassContext& ctx);
        void RenderShadowPass(Nova::Core::Renderer::RHI::IPassContext& ctx);

        std::unique_ptr<Nova::Core::Renderer::RHI::IRenderer> m_Renderer;
        Nova::Core::Scene::Scene* m_Scene{ nullptr };
        Nova::Core::Math::Camera* m_Camera{ nullptr };
        EditorSelection* m_Selection{ nullptr };

        Nova::Core::Renderer::RHI::RHI_TextureHandle m_SceneColor{};
        Nova::Core::Renderer::RHI::RHI_TextureHandle m_SceneDepth{};
        Nova::Core::Renderer::RHI::RHI_TextureHandle m_ShadowMaps{};
        Nova::Core::Renderer::RHI::RHI_TextureHandle m_SelectionMask{};
        Nova::Core::Renderer::RHI::RHI_TextureHandle m_SelectionBlurTemp{};
        Nova::Core::Renderer::RHI::RHI_TextureHandle m_SelectionBlurred{};

        Nova::Core::Renderer::RHI::RHI_ShaderHandle m_GridShader{};
        Nova::Core::Renderer::RHI::RHI_ShaderHandle m_SceneShader{};
        Nova::Core::Renderer::RHI::RHI_ShaderHandle m_ShadowShader{};
        Nova::Core::Renderer::RHI::RHI_ShaderHandle m_NormalsShader{};
        Nova::Core::Renderer::RHI::RHI_ShaderHandle m_PositionsShader{};
        Nova::Core::Renderer::RHI::RHI_ShaderHandle m_VertexColorShader{};
        Nova::Core::Renderer::RHI::RHI_ShaderHandle m_DepthShader{};
        Nova::Core::Renderer::RHI::RHI_ShaderHandle m_AABBShader{};
        Nova::Core::Renderer::RHI::RHI_ShaderHandle m_SelectionMaskShader{};
        Nova::Core::Renderer::RHI::RHI_ShaderHandle m_SelectionMaskOccludedShader{};
        Nova::Core::Renderer::RHI::RHI_ShaderHandle m_SelectionBlurHShader{};
        Nova::Core::Renderer::RHI::RHI_ShaderHandle m_SelectionBlurVShader{};
        Nova::Core::Renderer::RHI::RHI_ShaderHandle m_SelectionCompositeShader{};

        RenderDebugMode m_RenderDebugMode = RenderDebugMode::Lit;
        bool m_ShowGrid = true;
        bool m_ShowAABB = false;

        std::shared_ptr<Nova::Core::Renderer::RHI::RHI_Mesh> m_AABBWireframeMesh;
        std::vector<Nova::Core::Renderer::RHI::LightGPU> m_GpuLights;
    };

} // namespace Nova::App

#endif // EDITORRENDERER_H