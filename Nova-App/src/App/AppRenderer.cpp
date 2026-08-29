#include "App/AppRenderer.h"

#include <algorithm>
#include <cmath>
#include <filesystem>

#include "Asset/AssetManager.h"
#include "Asset/Assets/MeshAsset.h"
#include "Asset/Assets/ShaderAsset.h"
#include "Core/Assert.h"
#include "Core/Log.h"
#include "ECS/Components/LightComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/MeshRendererComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "Math/AABB.h"
#include "Math/Light.h"

namespace Nova::App {

    using namespace Nova::Core::Asset;
    using namespace Nova::Core::Asset::Assets;
    using namespace Nova::Core::ECS::Components;
    using namespace Nova::Core::Math;

    void AppRenderer::Initialize(
        Nova::Core::GraphicsAPI api,
        uint32_t width,
        uint32_t height)
    {
        Nova::Core::Renderer::RHI::RHI_SwapchainDesc swapDesc{};
        swapDesc.m_FramesInFlight = 3;
        swapDesc.m_CreateSurface = true;
        swapDesc.m_EnableSwapchain = true;
        swapDesc.m_PreferredPresentMode = Nova::Core::Renderer::RHI::RHI_PresentMode::Default;
        m_Renderer = Nova::Core::Renderer::RHI::IRenderer::Create(api, swapDesc);
        NV_ASSERT_MSG(m_Renderer, "Renderer is not initialized.");

        namespace RG = Nova::Core::Renderer::RHI;
        RG::RHI_RenderGraphBuilder fg;

        const auto backbuffer = fg.ImportTexture({
            width,
            height,
            RG::RHI_TextureFormat::RGBA8,
            RG::RHI_TextureUsage::ColorAttachment,
        }, RG::RHI_ResourceState::Present);

        RG::RHI_TextureHandle color = fg.CreateTexture({
            width,
            height,
            RG::RHI_TextureFormat::RGBA8,
            RG::RHI_TextureUsage::ColorAttachment | RG::RHI_TextureUsage::Sampled,
        });

        RG::RHI_TextureHandle depth = fg.CreateTexture({
            width,
            height,
            RG::RHI_TextureFormat::Depth32,
            RG::RHI_TextureUsage::DepthAttachment | RG::RHI_TextureUsage::Sampled,
        });

        RG::RHI_TextureHandle shadowMaps = fg.CreateTexture({
            Nova::Core::Renderer::RHI::SHADOW_MAP_RESOLUTION,
            Nova::Core::Renderer::RHI::SHADOW_MAP_RESOLUTION,
            RG::RHI_TextureFormat::Depth32,
            RG::RHI_TextureUsage::DepthAttachment | RG::RHI_TextureUsage::Sampled,
            Nova::Core::Renderer::RHI::MAX_SHADOW_MAPS,
            /*m_ResizeWithViewport*/ false,
            /*m_ComparisonSampler*/ true,
        });

        const RG::RHI_TextureUsage selectionRtUsage =
            RG::RHI_TextureUsage::ColorAttachment | RG::RHI_TextureUsage::Sampled;

        RG::RHI_TextureHandle selectionMask = fg.CreateTexture({
            width, height, RG::RHI_TextureFormat::RGBA8, selectionRtUsage,
            /*m_Layers*/ 1,
            /*m_ResizeWithViewport*/ true,
            /*m_ComparisonSampler*/ false,
            /*m_RegisterImGui*/ false,
        });
        RG::RHI_TextureHandle selectionBlurTemp = fg.CreateTexture({
            width, height, RG::RHI_TextureFormat::RGBA8, selectionRtUsage,
            /*m_Layers*/ 1,
            /*m_ResizeWithViewport*/ true,
            /*m_ComparisonSampler*/ false,
            /*m_RegisterImGui*/ false,
        });
        RG::RHI_TextureHandle selectionBlurred = fg.CreateTexture({
            width, height, RG::RHI_TextureFormat::RGBA8, selectionRtUsage,
            /*m_Layers*/ 1,
            /*m_ResizeWithViewport*/ true,
            /*m_ComparisonSampler*/ false,
            /*m_RegisterImGui*/ false,
        });

        m_SceneColor = color;
        m_SceneDepth = depth;
        m_ShadowMaps = shadowMaps;
        m_SelectionMask = selectionMask;
        m_SelectionBlurTemp = selectionBlurTemp;
        m_SelectionBlurred = selectionBlurred;

        auto acquireShader = [](const std::filesystem::path& uri) {
            auto asset = AssetManager::Get().Acquire<ShaderAsset>(uri).GetAssetRef();
            if (!asset || !asset->Compile()) {
                NV_LOG_WARN(("Failed to compile shader asset: " + uri.generic_string()).c_str());
            }
            return asset;
        };

        auto gridVert = acquireShader("Editor://Shaders/Grid.vert.slang");
        auto gridFrag = acquireShader("Editor://Shaders/Grid.frag.slang");
        auto sceneVert = acquireShader("Engine://Shaders/Scene.vert.slang");
        auto sceneFrag = acquireShader("Engine://Shaders/Scene.frag.slang");
        auto shadowVert = acquireShader("Engine://Shaders/Shadow.vert.slang");
        auto normalsFrag = acquireShader("Engine://Shaders/NormalsDebug.frag.slang");
        auto positionsFrag = acquireShader("Engine://Shaders/PositionsDebug.frag.slang");
        auto vertexColorFrag = acquireShader("Engine://Shaders/VertexColor.frag.slang");
        auto depthFrag = acquireShader("Engine://Shaders/DepthDebug.frag.slang");
        auto selectionFullscreenVert = acquireShader("Editor://Shaders/SelectionFullscreen.vert.slang");
        auto selectionMaskVert = acquireShader("Editor://Shaders/SelectionMask.vert.slang");
        auto selectionMaskFrag = acquireShader("Editor://Shaders/SelectionMask.frag.slang");
        auto selectionMaskOccludedFrag = acquireShader("Editor://Shaders/SelectionMaskOccluded.frag.slang");
        auto selectionBlurHFrag = acquireShader("Editor://Shaders/SelectionOutlineBlurH.frag.slang");
        auto selectionBlurVFrag = acquireShader("Editor://Shaders/SelectionOutlineBlurV.frag.slang");
        auto selectionCompositeFrag = acquireShader("Editor://Shaders/SelectionOutlineComposite.frag.slang");

        m_GridShader = fg.RegisterShader({
            .m_Name = "Grid",
            .m_Vertex = gridVert,
            .m_Fragment = gridFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::FullscreenQuad,
            .m_AlphaBlend = true,
        });

        m_SceneShader = fg.RegisterShader({
            .m_Name = "Scene",
            .m_Vertex = sceneVert,
            .m_Fragment = sceneFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::Mesh,
        });
        m_ShadowShader = fg.RegisterShader({
            .m_Name = "Shadow",
            .m_Vertex = shadowVert,
            .m_Fragment = nullptr,
            .m_VertexLayout = RG::RHI_VertexLayout::Mesh,
            .m_AlphaBlend = false,
            .m_DepthTest = true,
            .m_DepthWrite = true,
            .m_DepthOnly = true,
            .m_CullMode = RG::RHI_CullMode::Front,
            .m_DepthBiasConstant = 0.5f,
            .m_DepthBiasSlope = 1.0f,
        });
        m_NormalsShader = fg.RegisterShader({
            .m_Name = "NormalsDebug",
            .m_Vertex = sceneVert,
            .m_Fragment = normalsFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::Mesh,
        });
        m_PositionsShader = fg.RegisterShader({
            .m_Name = "PositionsDebug",
            .m_Vertex = sceneVert,
            .m_Fragment = positionsFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::Mesh,
        });
        m_VertexColorShader = fg.RegisterShader({
            .m_Name = "VertexColor",
            .m_Vertex = sceneVert,
            .m_Fragment = vertexColorFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::Mesh,
        });
        m_DepthShader = fg.RegisterShader({
            .m_Name = "DepthDebug",
            .m_Vertex = sceneVert,
            .m_Fragment = depthFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::Mesh,
        });
        m_AABBShader = fg.RegisterShader({
            .m_Name = "AABBDebug",
            .m_Vertex = sceneVert,
            .m_Fragment = vertexColorFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::Mesh,
            .m_PrimitiveTopology = RG::RHI_PrimitiveTopology::Lines,
            .m_DepthTest = false,
            .m_DepthWrite = false,
            .m_CullMode = RG::RHI_CullMode::None,
        });
        m_SelectionMaskShader = fg.RegisterShader({
            .m_Name = "SelectionMask",
            .m_Vertex = selectionMaskVert,
            .m_Fragment = selectionMaskFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::Mesh,
            .m_DepthTest = false,
            .m_DepthWrite = false,
            .m_CullMode = RG::RHI_CullMode::Back,
        });
        m_SelectionMaskOccludedShader = fg.RegisterShader({
            .m_Name = "SelectionMaskOccluded",
            .m_Vertex = selectionMaskVert,
            .m_Fragment = selectionMaskOccludedFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::Mesh,
            .m_DepthTest = true,
            .m_DepthWrite = false,
            .m_CullMode = RG::RHI_CullMode::Back,
            .m_DepthCompare = RG::RHI_DepthCompare::Greater,
        });
        m_SelectionBlurHShader = fg.RegisterShader({
            .m_Name = "SelectionBlurH",
            .m_Vertex = selectionFullscreenVert,
            .m_Fragment = selectionBlurHFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::FullscreenQuad,
            .m_DepthTest = false,
            .m_DepthWrite = false,
            .m_CullMode = RG::RHI_CullMode::None,
        });
        m_SelectionBlurVShader = fg.RegisterShader({
            .m_Name = "SelectionBlurV",
            .m_Vertex = selectionFullscreenVert,
            .m_Fragment = selectionBlurVFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::FullscreenQuad,
            .m_DepthTest = false,
            .m_DepthWrite = false,
            .m_CullMode = RG::RHI_CullMode::None,
        });
        m_SelectionCompositeShader = fg.RegisterShader({
            .m_Name = "SelectionComposite",
            .m_Vertex = selectionFullscreenVert,
            .m_Fragment = selectionCompositeFrag,
            .m_VertexLayout = RG::RHI_VertexLayout::FullscreenQuad,
            .m_AlphaBlend = true,
            .m_AdditiveBlend = true,
            .m_DepthTest = false,
            .m_DepthWrite = false,
            .m_CullMode = RG::RHI_CullMode::None,
        });

        fg.AddPass("Shadow",
            [&](RG::RHI_PassBuilder& b) {
                b.Write(shadowMaps);
            },
            [this](RG::IPassContext& ctx) {
                RenderShadowPass(ctx);
            });

        fg.AddPass("Grid",
            [&](RG::RHI_PassBuilder& b) {
                b.Write(color);
                if (depth.IsValid())
                    b.Write(depth);
            },
            [this](RG::IPassContext& ctx) {
                if (!m_ShowGrid)
                    return;
                auto* shader = ctx.GetShader(m_GridShader);
                if (!shader) return;
                const glm::vec3 resolution(ctx.GetRenderWidth(), ctx.GetRenderHeight(), 1.0f);
                shader->SetParameter("iResolution", resolution);
                ctx.DrawFullscreen(m_GridShader);
            });

        fg.AddPass("Scene",
            [&](RG::RHI_PassBuilder& b) {
                b.Read(color);
                b.Write(color);
                if (depth.IsValid()) {
                    b.Read(depth);
                    b.Write(depth);
                }
                b.Read(shadowMaps);
            },
            [this](RG::IPassContext& ctx) {
                RenderScene(ctx);
                if (m_ShowAABB)
                    RenderAABBs(ctx);
            });

        fg.AddPass("SelectionMask",
            [&](RG::RHI_PassBuilder& b) {
                b.Write(selectionMask);
                if (depth.IsValid()) {
                    b.Read(depth);
                    b.Write(depth);
                }
            },
            [this](RG::IPassContext& ctx) {
                RenderSelectionMask(ctx);
            });

        fg.AddPass("SelectionBlurH",
            [&](RG::RHI_PassBuilder& b) {
                b.Read(selectionMask);
                b.Write(selectionBlurTemp);
                if (depth.IsValid()) {
                    b.Read(depth);
                    b.Write(depth);
                }
            },
            [this](RG::IPassContext& ctx) {
                RenderSelectionBlur(ctx, m_SelectionBlurHShader);
            });

        fg.AddPass("SelectionBlurV",
            [&](RG::RHI_PassBuilder& b) {
                b.Read(selectionBlurTemp);
                b.Write(selectionBlurred);
                if (depth.IsValid()) {
                    b.Read(depth);
                    b.Write(depth);
                }
            },
            [this](RG::IPassContext& ctx) {
                RenderSelectionBlur(ctx, m_SelectionBlurVShader);
            });

        fg.AddPass("SelectionComposite",
            [&](RG::RHI_PassBuilder& b) {
                b.Read(selectionMask);
                b.Read(selectionBlurred);
                b.Read(color);
                b.Write(color);
                if (depth.IsValid()) {
                    b.Read(depth);
                    b.Write(depth);
                }
            },
            [this](RG::IPassContext& ctx) {
                RenderSelectionComposite(ctx);
            });

        fg.AddPass("UI",
            [&](RG::RHI_PassBuilder& b) {
                b.Write(backbuffer);
                b.PresentOnly();
            },
            [](RG::IPassContext& /*ctx*/) {
            });

        m_Renderer->SetRenderGraph(fg.Build(api));

        if (auto* graph = m_Renderer->GetRenderGraph()) {
            graph->BindEngineShadowMaps(m_ShadowMaps);
            BindSelectionOutlineTextures();
        }

        m_AABBWireframeMesh = Nova::Core::Math::CreateUnitAABBWireframeMesh(glm::vec3(0.0f, 1.0f, 0.0f));
    }

    void AppRenderer::Shutdown() {
        if (m_Renderer) {
            m_Renderer->Destroy();
            m_Renderer.reset();
        }
    }

    void AppRenderer::BeginFrame() {
        NV_ASSERT_MSG(m_Renderer, "Renderer is not initialized.");
        m_Renderer->BeginFrame();
    }

    void AppRenderer::RenderFrame() {
        NV_ASSERT_MSG(m_Renderer, "Renderer is not initialized.");
        m_Renderer->RenderFrame();
    }

    void AppRenderer::EndFrame() {
        NV_ASSERT_MSG(m_Renderer, "Renderer is not initialized.");
        m_Renderer->EndFrame();
    }

    void AppRenderer::Resize(int width, int height) {
        NV_ASSERT_MSG(m_Renderer, "Renderer is not initialized.");
        m_Renderer->Resize(width, height);
    }

    void AppRenderer::BindFrame(
        Nova::Core::Scene::Scene& scene,
        Nova::Core::Math::Camera& camera,
        EditorSelection* selection)
    {
        m_Scene = &scene;
        m_Camera = &camera;
        m_Selection = selection;
    }

    void AppRenderer::RebindAfterResize() {
        if (!m_Renderer)
            return;
        if (auto* graph = m_Renderer->GetRenderGraph()) {
            graph->BindEngineShadowMaps(m_ShadowMaps);
            BindSelectionOutlineTextures();
        }
    }

    Nova::Core::Renderer::RHI::RHI_ShaderHandle AppRenderer::GetActiveSceneShader() const {
        switch (m_RenderDebugMode) {
            case RenderDebugMode::Normals:     return m_NormalsShader;
            case RenderDebugMode::Positions:   return m_PositionsShader;
            case RenderDebugMode::VertexColor: return m_VertexColorShader;
            case RenderDebugMode::Depth:       return m_DepthShader;
            case RenderDebugMode::Lit:
            default:
                return m_SceneShader;
        }
    }

    void AppRenderer::PushGlobals(
        float elapsedTime,
        float deltaTime,
        uint32_t& frameIndex,
        const glm::vec2& viewportSize)
    {
        NV_ASSERT_MSG(m_Renderer, "Renderer is not initialized.");
        NV_ASSERT_MSG(m_Camera, "Camera is not bound for this frame.");

        const glm::mat4 view = m_Camera->GetViewMatrix();
        const glm::mat4 proj = m_Camera->GetProjectionMatrix();
        const glm::mat4 viewProj = proj * view;
        const glm::mat4 invViewProj = glm::inverse(viewProj);
        const int lightCount = static_cast<int>(m_GpuLights.size());

        auto setGlobals = [elapsedTime, deltaTime, &frameIndex, viewportSize, view, proj, viewProj, invViewProj, lightCount](
            Nova::Core::Renderer::RHI::IShaders* shader)
        {
            if (!shader) return;
            shader->SetParameter("iTime", elapsedTime);
            shader->SetParameter("iTimeDelta", deltaTime);
            shader->SetParameter("iFrameRate", deltaTime > 0.0f ? 1.0f / deltaTime : 0.0f);
            shader->SetParameter("iFrame", static_cast<int>(frameIndex++));
            shader->SetParameter("iResolution", glm::vec3(viewportSize.x, viewportSize.y, 1.0f));
            shader->SetParameter("view", view);
            shader->SetParameter("proj", proj);
            shader->SetParameter("viewProj", viewProj);
            shader->SetParameter("invViewProj", invViewProj);
            shader->SetParameter("lightCount", lightCount);
        };

        if (auto* graph = m_Renderer->GetRenderGraph()) {
            setGlobals(graph->GetShader(m_GridShader));
            setGlobals(graph->GetShader(m_SceneShader));
            setGlobals(graph->GetShader(m_ShadowShader));
            setGlobals(graph->GetShader(m_NormalsShader));
            setGlobals(graph->GetShader(m_PositionsShader));
            setGlobals(graph->GetShader(m_VertexColorShader));
            setGlobals(graph->GetShader(m_DepthShader));
            setGlobals(graph->GetShader(m_AABBShader));
            setGlobals(graph->GetShader(m_SelectionMaskShader));
            setGlobals(graph->GetShader(m_SelectionMaskOccludedShader));
        }
    }

    void AppRenderer::RenderScene(Nova::Core::Renderer::RHI::IPassContext& ctx) {
        NV_ASSERT_MSG(m_Renderer, "Renderer is not initialized.");
        NV_ASSERT_MSG(m_Camera, "Camera is not bound for this frame.");
        NV_ASSERT_MSG(m_Scene, "Scene is not bound for this frame.");

        const auto shaderHandle = GetActiveSceneShader();
        auto* sceneShader = ctx.GetShader(shaderHandle);
        if (!sceneShader) return;

        auto& registry = m_Scene->GetRegistry();
        auto viewMeshes = registry.view<TransformComponent, MeshRendererComponent>();
        for (auto entity : viewMeshes) {
            auto& tc = viewMeshes.get<TransformComponent>(entity);
            auto& mrc = viewMeshes.get<MeshRendererComponent>(entity);

            if (!mrc.m_MeshAsset || !mrc.m_MeshAsset->IsLoaded())
                continue;

            auto cpuMesh = mrc.m_MeshAsset->GetCPUMesh();
            if (!cpuMesh)
                continue;

            sceneShader->SetParameter("model", tc.GetTransform());
            sceneShader->SetParameter("u_CameraPos", m_Camera->m_LookFrom);

            sceneShader->SetParameter("base", mrc.m_Material.m_Base);
            sceneShader->SetParameter("baseColor", mrc.m_Material.m_BaseColor);
            sceneShader->SetParameter("diffuseRoughness", mrc.m_Material.m_DiffuseRoughness);
            sceneShader->SetParameter("metalness", mrc.m_Material.m_Metalness);
            sceneShader->SetParameter("metalColor", mrc.m_Material.m_MetalColor);
            sceneShader->SetParameter("specular", mrc.m_Material.m_Specular);
            sceneShader->SetParameter("specularColor", mrc.m_Material.m_SpecularColor);
            sceneShader->SetParameter("specularRoughness", mrc.m_Material.m_SpecularRoughness);
            sceneShader->SetParameter("specularIOR", mrc.m_Material.m_SpecularIOR);
            sceneShader->SetParameter("specularAnisotropy", mrc.m_Material.m_SpecularAnisotropy);
            sceneShader->SetParameter("specularRotation", mrc.m_Material.m_SpecularRotation);
            sceneShader->SetParameter("transmission", mrc.m_Material.m_Transmission);
            sceneShader->SetParameter("transmissionColor", mrc.m_Material.m_TransmissionColor);
            sceneShader->SetParameter("subsurface", mrc.m_Material.m_Subsurface);
            sceneShader->SetParameter("subsurfaceColor", mrc.m_Material.m_SubsurfaceColor);
            sceneShader->SetParameter("subsurfaceRadius", mrc.m_Material.m_SubsurfaceRadius);
            sceneShader->SetParameter("subsurfaceScale", mrc.m_Material.m_SubsurfaceScale);
            sceneShader->SetParameter("subsurfaceAnisotropy", mrc.m_Material.m_SubsurfaceAnisotropy);
            sceneShader->SetParameter("sheen", mrc.m_Material.m_Sheen);
            sceneShader->SetParameter("sheenColor", mrc.m_Material.m_SheenColor);
            sceneShader->SetParameter("sheenRoughness", mrc.m_Material.m_SheenRoughness);
            sceneShader->SetParameter("coat", mrc.m_Material.m_Coat);
            sceneShader->SetParameter("coatColor", mrc.m_Material.m_CoatColor);
            sceneShader->SetParameter("coatRoughness", mrc.m_Material.m_CoatRoughness);
            sceneShader->SetParameter("coatAnisotropy", mrc.m_Material.m_CoatAnisotropy);
            sceneShader->SetParameter("coatRotation", mrc.m_Material.m_CoatRotation);
            sceneShader->SetParameter("coatIOR", mrc.m_Material.m_CoatIOR);
            sceneShader->SetParameter("coatAffectColor", mrc.m_Material.m_CoatAffectColor);
            sceneShader->SetParameter("coatAffectRoughness", mrc.m_Material.m_CoatAffectRoughness);
            sceneShader->SetParameter("emission", mrc.m_Material.m_Emission);
            sceneShader->SetParameter("emissionColor", mrc.m_Material.m_EmissionColor);
            sceneShader->SetParameter("opacity", mrc.m_Material.m_Opacity);
            sceneShader->SetParameter("thinWalled", mrc.m_Material.m_ThinWalled);
            sceneShader->SetParameter("isOpaque", static_cast<int>(mrc.m_Material.m_IsOpaque));

            ctx.BindShader(shaderHandle);

            Nova::Core::Renderer::RHI::RHI_DrawIndexedCommand cmd{};
            cmd.m_Mesh = cpuMesh;
            cmd.m_Topology = Nova::Core::Renderer::RHI::RHI_PrimitiveTopology::Triangles;
            cmd.m_IndexType = Nova::Core::Renderer::RHI::RHI_IndexType::UInt32;
            cmd.m_IndexCount = static_cast<uint32_t>(cpuMesh->GetIndices().size());
            ctx.DrawIndexed(cmd);
        }
    }

    void AppRenderer::RenderSelectionMask(Nova::Core::Renderer::RHI::IPassContext& ctx) {
        if (!m_Selection || !m_Scene || m_Selection->Empty())
            return;

        auto& registry = m_Scene->GetRegistry();
        auto& selected = m_Selection->GetEntities();

        std::erase_if(selected, [&](entt::entity entity) {
            return !registry.valid(entity);
        });
        if (selected.empty())
            return;

        for (entt::entity entity : selected) {
            auto* tc = registry.try_get<TransformComponent>(entity);
            auto* mrc = registry.try_get<MeshRendererComponent>(entity);
            if (!tc || !mrc || !mrc->m_MeshAsset || !mrc->m_MeshAsset->IsLoaded())
                continue;

            auto cpuMesh = mrc->m_MeshAsset->GetCPUMesh();
            if (!cpuMesh)
                continue;

            const glm::mat4 model = tc->GetTransform();

            Nova::Core::Renderer::RHI::RHI_DrawIndexedCommand cmd{};
            cmd.m_Mesh = cpuMesh;
            cmd.m_Topology = Nova::Core::Renderer::RHI::RHI_PrimitiveTopology::Triangles;
            cmd.m_IndexType = Nova::Core::Renderer::RHI::RHI_IndexType::UInt32;
            cmd.m_IndexCount = static_cast<uint32_t>(cpuMesh->GetIndices().size());

            if (auto* mask = ctx.GetShader(m_SelectionMaskShader)) {
                mask->SetParameter("model", model);
                ctx.BindShader(m_SelectionMaskShader);
                ctx.DrawIndexed(cmd);
            }

            if (auto* occluded = ctx.GetShader(m_SelectionMaskOccludedShader)) {
                occluded->SetParameter("model", model);
                ctx.BindShader(m_SelectionMaskOccludedShader);
                ctx.DrawIndexed(cmd);
            }
        }
    }

    void AppRenderer::RenderSelectionBlur(
        Nova::Core::Renderer::RHI::IPassContext& ctx,
        Nova::Core::Renderer::RHI::RHI_ShaderHandle blurShader)
    {
        if (!m_Selection || m_Selection->Empty() || !blurShader.IsValid())
            return;
        ctx.DrawFullscreen(blurShader);
    }

    void AppRenderer::RenderSelectionComposite(Nova::Core::Renderer::RHI::IPassContext& ctx) {
        if (!m_Selection || m_Selection->Empty() || !m_SelectionCompositeShader.IsValid())
            return;
        ctx.DrawFullscreen(m_SelectionCompositeShader);
    }

    void AppRenderer::BindSelectionOutlineTextures() {
        auto* graph = m_Renderer ? m_Renderer->GetRenderGraph() : nullptr;
        if (!graph)
            return;

        auto bindOutlineTexture = [&](
            Nova::Core::Renderer::RHI::RHI_ShaderHandle shaderHandle,
            const char* textureName,
            const char* samplerName,
            Nova::Core::Renderer::RHI::RHI_TextureHandle textureHandle)
        {
            auto* shader = graph->GetShader(shaderHandle);
            if (!shader)
                return;
            uint64_t imageView = 0;
            uint64_t sampler = 0;
            if (!graph->GetSampledTextureNativeHandles(textureHandle, imageView, sampler))
                return;
            shader->BindSampledTexture(textureName, samplerName, imageView, sampler);
        };

        bindOutlineTexture(
            m_SelectionBlurHShader, "outline.source", "outline.sourceSampler", m_SelectionMask);
        bindOutlineTexture(
            m_SelectionBlurVShader, "outline.source", "outline.sourceSampler", m_SelectionBlurTemp);
        bindOutlineTexture(
            m_SelectionCompositeShader,
            "outline.selectionMask", "outline.selectionMaskSampler",
            m_SelectionMask);
        bindOutlineTexture(
            m_SelectionCompositeShader,
            "outline.selectionBlurred", "outline.selectionBlurredSampler",
            m_SelectionBlurred);
    }

    void AppRenderer::RenderAABBs(Nova::Core::Renderer::RHI::IPassContext& ctx) {
        if (!m_AABBWireframeMesh || !m_Scene || !m_Camera)
            return;

        auto* aabbShader = ctx.GetShader(m_AABBShader);
        if (!aabbShader)
            return;

        auto& registry = m_Scene->GetRegistry();
        auto view = registry.view<TransformComponent, MeshComponent>();

        for (auto entity : view) {
            auto& tc = view.get<TransformComponent>(entity);
            auto& mc = view.get<MeshComponent>(entity);

            if (!mc.m_AABBTree.IsBuilt())
                continue;

            const glm::mat4 entityTransform = tc.GetTransform();
            const auto& nodes = mc.m_AABBTree.GetNodes();

            for (const auto& node : nodes) {
                const glm::mat4 model = entityTransform * Nova::Core::Math::AABBToModelMatrix(node.m_Bounds);
                aabbShader->SetParameter("model", model);
                aabbShader->SetParameter("u_CameraPos", m_Camera->m_LookFrom);

                ctx.BindShader(m_AABBShader);

                Nova::Core::Renderer::RHI::RHI_DrawIndexedCommand cmd{};
                cmd.m_Mesh = m_AABBWireframeMesh;
                cmd.m_Topology = Nova::Core::Renderer::RHI::RHI_PrimitiveTopology::Lines;
                cmd.m_IndexType = Nova::Core::Renderer::RHI::RHI_IndexType::UInt32;
                cmd.m_IndexCount = static_cast<uint32_t>(m_AABBWireframeMesh->GetIndices().size());
                ctx.DrawIndexed(cmd);
            }
        }
    }

    void AppRenderer::UploadLights() {
        NV_ASSERT_MSG(m_Scene, "Scene is not bound for this frame.");

        m_GpuLights.clear();
        auto& registry = m_Scene->GetRegistry();
        auto view = registry.view<TransformComponent, LightComponent>();

        int nextShadow = 0;
        for (auto entity : view) {
            if (m_GpuLights.size() >= Nova::Core::Renderer::RHI::MAX_LIGHTS)
                break;

            auto& tc = view.get<TransformComponent>(entity);
            auto& lc = view.get<LightComponent>(entity);
            if (!lc.m_Light)
                continue;

            const Light& light = *lc.m_Light;
            const glm::vec3 position = tc.m_Translation;
            const glm::vec3 travelDir = glm::length(light.m_Direction) > 1e-6f
                ? glm::normalize(light.m_Direction)
                : glm::vec3(0.0f, -1.0f, 0.0f);

            Nova::Core::Renderer::RHI::LightGPU gpu{};
            gpu.m_Type = static_cast<int>(light.m_Type);
            gpu.m_Intensity = light.m_Intensity;
            gpu.m_Color = light.m_Color;
            gpu.m_Range = light.m_Range;
            gpu.m_Direction = travelDir;
            gpu.m_InnerConeCos = light.InnerCos();
            gpu.m_Position = position;
            gpu.m_OuterConeCos = light.OuterCos();
            gpu.m_ShadowBiasConstant = light.m_ShadowBiasConstant;
            gpu.m_ShadowBiasSlope = light.m_ShadowBiasSlope;
            gpu.m_ShadowNormalBias = light.m_ShadowNormalBias;
            gpu.m_CastShadow = 0;
            gpu.m_ShadowMapIndex = -1;

            const bool canShadow = light.m_LightShadow
                && (light.m_Type == LightType::Directional || light.m_Type == LightType::Spot)
                && nextShadow < static_cast<int>(Nova::Core::Renderer::RHI::MAX_SHADOW_MAPS);
            if (canShadow) {
                gpu.m_CastShadow = 1;
                gpu.m_ShadowMapIndex = nextShadow++;
                gpu.m_LightViewProj = BuildLightViewProj(
                    light.m_Type, position, travelDir, light.m_Range, light.m_OuterCone,
                    Nova::Core::Renderer::RHI::SHADOW_DIR_ORTHO_HALF_EXTENT);
            }

            m_GpuLights.push_back(gpu);
        }

        auto* graph = m_Renderer ? m_Renderer->GetRenderGraph() : nullptr;
        const auto* engine = graph ? graph->GetEngineParameterBlock() : nullptr;
        if (!engine || !engine->m_Lights.IsValid())
            return;

        if (!m_GpuLights.empty()) {
            m_Renderer->UpdateGpuBuffer(
                engine->m_Lights,
                m_GpuLights.data(),
                sizeof(Nova::Core::Renderer::RHI::LightGPU) * m_GpuLights.size(),
                0);
        }
    }

    void AppRenderer::RenderShadowPass(Nova::Core::Renderer::RHI::IPassContext& ctx) {
        if (!m_Renderer || !m_ShadowMaps.IsValid() || !m_Scene)
            return;

        for (uint32_t layer = 0; layer < Nova::Core::Renderer::RHI::MAX_SHADOW_MAPS; ++layer) {
            ctx.BeginDepthLayer(m_ShadowMaps, layer, true);
            ctx.EndDepthLayer();
        }

        auto* shadowShader = ctx.GetShader(m_ShadowShader);
        if (!shadowShader)
            return;

        auto& registry = m_Scene->GetRegistry();
        auto meshView = registry.view<TransformComponent, MeshRendererComponent>();

        for (const auto& light : m_GpuLights) {
            if (!light.m_CastShadow || light.m_ShadowMapIndex < 0)
                continue;

            ctx.BeginDepthLayer(m_ShadowMaps, static_cast<uint32_t>(light.m_ShadowMapIndex), true);
            const float dirLen2 = glm::dot(light.m_Direction, light.m_Direction);
            const float angleFactor = dirLen2 > 1e-12f
                ? std::max(std::abs(light.m_Direction.y) / std::sqrt(dirLen2), 0.35f)
                : 1.0f;
            const float typeScale = (light.m_Type == static_cast<int>(LightType::Spot)) ? 0.35f : 1.0f;
            ctx.SetDepthBias(
                light.m_ShadowBiasConstant * angleFactor * typeScale,
                light.m_ShadowBiasSlope * angleFactor * typeScale);

            for (auto entity : meshView) {
                auto& tc = meshView.get<TransformComponent>(entity);
                auto& mrc = meshView.get<MeshRendererComponent>(entity);
                if (!mrc.m_MeshAsset || !mrc.m_MeshAsset->IsLoaded())
                    continue;
                auto cpuMesh = mrc.m_MeshAsset->GetCPUMesh();
                if (!cpuMesh)
                    continue;

                const glm::mat4 model = tc.GetTransform();
                shadowShader->SetParameter("model", model);
                shadowShader->SetParameter("viewProj", light.m_LightViewProj);
                ctx.BindShader(m_ShadowShader);

                Nova::Core::Renderer::RHI::RHI_DrawIndexedCommand cmd{};
                cmd.m_Mesh = cpuMesh;
                cmd.m_Topology = Nova::Core::Renderer::RHI::RHI_PrimitiveTopology::Triangles;
                cmd.m_IndexType = Nova::Core::Renderer::RHI::RHI_IndexType::UInt32;
                cmd.m_IndexCount = static_cast<uint32_t>(cpuMesh->GetIndices().size());
                ctx.DrawIndexed(cmd);
            }

            ctx.EndDepthLayer();
        }
    }

} // namespace Nova::App