#ifndef GPU_INSTANCING_LAYER_H
#define GPU_INSTANCING_LAYER_H

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "Core/Layer.h"
#include "Core/UUID.h"
#include "Events/ApplicationEvents.h"
#include "Events/Event.h"
#include "Events/InputEvents.h"
#include "Math/AABB.h"
#include "Renderer/RHI/RHI_Mesh.h"
#include "Renderer/RHI/RHI_Renderer.h"
#include "Renderer/RHI/RHI_ShaderUniforms.h"

/**
 * Thousands of randomly placed and rotated meshes, drawn either with one instanced draw call per
 * mesh (GPU instancing) or with one draw call per object. Both modes read the same per-instance
 * data from `nova.instances`, so the difference measured is the draw call overhead.
 *
 * Scene-level batching lives in SceneQuery::BuildRenderBatches; this test builds instance lists
 * locally so it can stress the GPU path without wiring a full ECS Scene.
 */
class GPUInstancingLayer : public Nova::Core::Layer {
public:
    explicit GPUInstancingLayer();
    ~GPUInstancingLayer() override = default;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float dt) override;
    void OnBegin() override;
    void OnRender() override;
    void OnEnd() override;
    void OnImGuiRender() override;
    void OnEvent(Nova::Core::Events::Event& e) override;

private:
    struct MeshSlot {
        const char* m_Name = "";
        std::shared_ptr<Nova::Core::Renderer::RHI::RHI_Mesh> m_Mesh;
        Nova::Core::UUID m_ID = Nova::Core::kInvalidUUID;
        Nova::Core::Math::AABB m_LocalBounds;
        bool m_Enabled = true;
        uint32_t m_TriangleCount = 0;
    };

    struct SceneObject {
        glm::vec3 m_Position{ 0.0f };
        glm::vec3 m_RotationAxis{ 0.0f, 1.0f, 0.0f };
        float m_RotationAngle = 0.0f; // radians
        float m_RotationSpeed = 0.0f; // radians per second
        float m_Scale = 1.0f;
        uint32_t m_MaterialIndex = 0;
        uint32_t m_MixedMeshSlot = 0;
    };

    struct InstanceBatch {
        Nova::Core::UUID m_MeshID = Nova::Core::kInvalidUUID;
        std::shared_ptr<Nova::Core::Renderer::RHI::RHI_Mesh> m_Mesh;
        uint32_t m_FirstInstance = 0;
        uint32_t m_InstanceCount = 0;
    };

    bool CreateRenderer();
    bool LoadMeshes();
    void GenerateMaterials();
    void GenerateLights();
    void GenerateObjects();
    void RegenerateScene();
    void UpdateTransforms();
    void RebuildBatches();
    void PrepareInstances();
    bool GrowEngineBuffer(Nova::Core::Renderer::RHI::RHI_GpuBufferHandle handle, uint32_t elementCount);

    void RenderShadowPass(Nova::Core::Renderer::RHI::IPassContext& ctx);
    void RenderScenePass(Nova::Core::Renderer::RHI::IPassContext& ctx);
    void UploadFrameData(uint32_t width, uint32_t height);
    uint32_t DrawBatches(Nova::Core::Renderer::RHI::IPassContext& ctx,
                         Nova::Core::Renderer::RHI::RHI_ShaderHandle shader,
                         const std::vector<InstanceBatch>& batches,
                         Nova::Core::Renderer::RHI::RHI_PrimitiveTopology topology);

    const Nova::Core::Renderer::RHI::RHI_EngineParameterBlock* GetEngine() const;
    const MeshSlot& GetObjectMesh(const SceneObject& object) const;
    const char* GetMeshName(Nova::Core::UUID meshID) const;
    uint32_t GetMeshTriangleCount(Nova::Core::UUID meshID) const;
    std::vector<uint32_t> GetEnabledMeshIndices() const;
    glm::vec3 GetCameraPosition() const;
    void PushFrameTimeSample(float ms);

    bool OnMouseButtonPressed(Nova::Core::Events::MouseButtonPressedEvent& e);
    bool OnMouseButtonReleased(Nova::Core::Events::MouseButtonReleasedEvent& e);
    bool OnMouseMoved(Nova::Core::Events::MouseMovedEvent& e);
    bool OnMouseScrolled(Nova::Core::Events::MouseScrolledEvent& e);
    bool OnWindowResized(Nova::Core::Events::WindowResizeEvent& e);

    static constexpr uint32_t kFramesInFlight = 3;
    static constexpr uint32_t kMeshSlotCount = 7;
    static constexpr size_t kFrameTimeHistory = 120;

    std::unique_ptr<Nova::Core::Renderer::RHI::IRenderer> m_Renderer;
    Nova::Core::Renderer::RHI::RHI_ShaderHandle m_SceneShader;
    Nova::Core::Renderer::RHI::RHI_ShaderHandle m_AABBShader;
    Nova::Core::Renderer::RHI::RHI_TextureHandle m_SceneColor;
    Nova::Core::Renderer::RHI::RHI_TextureHandle m_SceneDepth;
    Nova::Core::Renderer::RHI::RHI_TextureHandle m_ShadowMaps;

    std::array<MeshSlot, kMeshSlotCount> m_MeshSlots;
    MeshSlot m_AABBMesh;

    std::vector<SceneObject> m_Objects;
    std::vector<glm::mat4> m_ObjectModels;
    std::vector<Nova::Core::Math::AABB> m_ObjectBounds;
    std::vector<Nova::Core::Renderer::RHI::MaterialGPU> m_Materials;
    std::vector<Nova::Core::Renderer::RHI::LightGPU> m_Lights;
    float m_SceneHalfExtent = 1.0f;

    std::vector<Nova::Core::Renderer::RHI::InstanceGPU> m_ObjectInstances;
    std::vector<InstanceBatch> m_ObjectBatches;
    std::vector<Nova::Core::Renderer::RHI::InstanceGPU> m_AABBInstances;
    std::vector<InstanceBatch> m_AABBBatches;
    uint32_t m_AABBFirstInstance = 0;

    bool m_TransformsDirty = true;
    bool m_BatchesDirty = true;
    uint32_t m_PendingUploads = 0;

    bool m_UseInstancing = true;
    bool m_ShowAABBs = false;
    bool m_Animate = true;
    bool m_AutoOrbit = false;
    int m_ObjectCount = 10000;
    int m_MaterialCount = 64;
    float m_Spacing = 2.0f;
    uint32_t m_Seed = 1337;
    float m_AnimationTime = 0.0f;

    float m_CameraYaw = 0.7f;
    float m_CameraPitch = 0.45f;
    float m_CameraDistance = 100.0f;
    bool m_Orbiting = false;
    glm::vec2 m_LastMouse{ 0.0f };

    uint32_t m_DrawCalls = 0;
    uint32_t m_InstancesDrawn = 0;
    uint64_t m_TrianglesDrawn = 0;
    uint32_t m_UploadedInstances = 0;
    uint32_t m_UploadedMaterials = 0;

    float m_PrepareMs = 0.0f;
    float m_RecordMs = 0.0f;
    float m_AnimationMs = 0.0f;
    float m_BatcherUpdateMs = 0.0f;
    float m_LastBatchRebuildMs = 0.0f;
    float m_SceneGenMs = 0.0f;
    uint32_t m_BatchRebuildCount = 0;

    std::array<float, kFrameTimeHistory> m_FrameTimeHistory{};
    size_t m_FrameTimeWrite = 0;
    size_t m_FrameTimeCount = 0;

    float m_ElapsedTime = 0.0f;
    float m_DeltaTime = 0.0f;
    int m_FrameIndex = 0;
    bool m_ResizePending = false;
    int m_PendingWidth = 0;
    int m_PendingHeight = 0;
};

#endif // GPU_INSTANCING_LAYER_H