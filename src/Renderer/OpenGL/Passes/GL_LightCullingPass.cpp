#include "Renderer/OpenGL/GL_RenderPass.hpp"
#include "Renderer/OpenGL/GL_RenderPassCtx.hpp"
#include "Renderer/OpenGL/GL_Shader.hpp"

#include <iostream>
#include <glm/gtc/type_ptr.hpp>

namespace Nova::Renderer::OpenGL {

    float GL_LightCullingPass::LinearToNDC_Z(float zEye, float n, float f, const glm::mat4& proj) {
        // Inverse linearization formula: ndc = (f+n - 2nf/z) / (f - n)
        // (Standard OpenGL perspective assumption)
        float ndc = (f + n - 2.0f * f * n / zEye) / (f - n);
        // Clamp for numerical stability
        return glm::clamp(ndc, -1.0f, 1.0f);
    }

    void GL_LightCullingPass::BuildClusterAABBs_ViewSpace(const GL_RenderPassCtx& ctx, std::vector<Math::AABB>& out) {
        out.clear();
        out.resize(GetNumClusters());

        // Inverse projection for unprojecting NDC -> view space
        glm::mat4 invProjection = glm::inverse(ctx.m_Projection);

        // Screen slicing (NDC)
        // x in [-1,1], y in [-1,1], z in [-1,1] (OpenGL)
        auto idx = [this](int x, int y, int z) {
            return size_t(z) * size_t(m_ClusterDimX) * size_t(m_ClusterDimY)
                 + size_t(y) * size_t(m_ClusterDimX)
                 + size_t(x);
        };

        // Logarithmic Z slicing
        auto zSlice = [&](int iz)->float {
            float a = float(iz) / float(m_ClusterDimZ);
            // logarithmic between near..far
            return ctx.m_Near * std::pow(ctx.m_Far / ctx.m_Near, a);
        };

        for (int iz = 0; iz < m_ClusterDimZ; ++iz) {
            float zNear = zSlice(iz);
            float zFar  = zSlice(iz + 1);

            // Convert to NDC
            float ndcNear = LinearToNDC_Z(zNear, ctx.m_Near, ctx.m_Far, ctx.m_Projection);
            float ndcFar  = LinearToNDC_Z(zFar,  ctx.m_Near, ctx.m_Far, ctx.m_Projection);

            for (int iy = 0; iy < m_ClusterDimY; ++iy) {
                float ny0 = -1.0f + 2.0f * (float(iy) / float(m_ClusterDimY));
                float ny1 = -1.0f + 2.0f * (float(iy + 1) / float(m_ClusterDimY));

                for (int ix = 0; ix < m_ClusterDimX; ++ix) {
                    float nx0 = -1.0f + 2.0f * (float(ix) / float(m_ClusterDimX));
                    float nx1 = -1.0f + 2.0f * (float(ix + 1) / float(m_ClusterDimX));

                    // 8 corners in NDC
                    glm::vec4 ndcCorners[8] = {
                        {nx0, ny0, ndcNear, 1.0f}, {nx1, ny0, ndcNear, 1.0f},
                        {nx0, ny1, ndcNear, 1.0f}, {nx1, ny1, ndcNear, 1.0f},
                        {nx0, ny0, ndcFar , 1.0f}, {nx1, ny0, ndcFar , 1.0f},
                        {nx0, ny1, ndcFar , 1.0f}, {nx1, ny1, ndcFar , 1.0f},
                    };

                    // Unproject -> view space
                    glm::vec3 vmin( std::numeric_limits<float>::max());
                    glm::vec3 vmax(-std::numeric_limits<float>::max());
                    for (int c=0;c<8;++c){
                        glm::vec4 v = invProjection * ndcCorners[c];
                        v /= v.w; // perspective divide
                        // OpenGL view convention: -Z is forward → z will be negative.
                        // We want AABBs in view space: no additional transform
                        vmin = glm::min(vmin, glm::vec3(v));
                        vmax = glm::max(vmax, glm::vec3(v));
                    }

                    // Range of positive zEye for convenience in shader:
                    // we convert min/max z (negative) to positive distances
                    // But we keep the AABB as is in view space (z negative)
                    Math::AABB aabb{ vmin, vmax };
                    out[idx(ix,iy,iz)] = aabb;
                }
            }
        }
    }

    void GL_LightCullingPass::UploadOrResizeBuffers(const GL_RenderPassCtx& ctx, int numClusters) {
        // Lights SSBO
        if (!m_LightsSSBO) glGenBuffers(1, &m_LightsSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_LightsSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GL_RenderPassCtx::GPULight) * ctx.m_NumberOfLights, nullptr, GL_DYNAMIC_DRAW);

        // Clusters SSBO
        if (!m_ClustersSSBO) glGenBuffers(1, &m_ClustersSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ClustersSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Math::AABB) * numClusters, nullptr, GL_DYNAMIC_DRAW);

        // Grid SSBO (offset,count)
        if (!m_GridSSBO) glGenBuffers(1, &m_GridSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_GridSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t) * 2 * numClusters, nullptr, GL_DYNAMIC_DRAW);

        // IndexList SSBO (numClusters * MaxPerCluster)
        const size_t indexListCount = (size_t)numClusters * (size_t)m_MaxLightsPerCluster;
        if (!m_IndexListSSBO) glGenBuffers(1, &m_IndexListSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_IndexListSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t) * indexListCount, nullptr, GL_DYNAMIC_DRAW);
    }

    void GL_LightCullingPass::Init() {
        // Compile shaders
        std::string computePath = std::string(SHADER_DIR) + "/lightCullingPass.comp";
        m_Program = LoadComputeShader(computePath);

        if (m_Program == 0) {
            std::cerr << "Failed to load/compile shaders!" << std::endl;
        }
    }

    void GL_LightCullingPass::Destroy() {
        if (m_Program) { glDeleteProgram(m_Program); m_Program = 0; }
        if (m_LightsSSBO)    { glDeleteBuffers(1, &m_LightsSSBO);    m_LightsSSBO = 0; }
        if (m_ClustersSSBO)  { glDeleteBuffers(1, &m_ClustersSSBO);  m_ClustersSSBO = 0; }
        if (m_GridSSBO)      { glDeleteBuffers(1, &m_GridSSBO);      m_GridSSBO = 0; }
        if (m_IndexListSSBO) { glDeleteBuffers(1, &m_IndexListSSBO); m_IndexListSSBO = 0; }
    }

    void GL_LightCullingPass::Execute(const GL_RenderPassCtx& ctx) {
        if (!ctx.m_Scene || m_Program==0) return;

        const size_t numClusters = GetNumClusters();

        m_ClusterAABBs.clear();
        BuildClusterAABBs_ViewSpace(ctx, m_ClusterAABBs);

        UploadOrResizeBuffers(ctx, (int)numClusters);

        // Upload lights
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_LightsSSBO);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GL_RenderPassCtx::GPULight) * (int)ctx.m_Lights.size(), ctx.m_Lights.data());

        // Upload clusters
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ClustersSSBO);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Math::AABB) * m_ClusterAABBs.size(), m_ClusterAABBs.data());

        {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_GridSSBO);
            std::vector<uint32_t> gridInit(numClusters * 2); 
            for (size_t i=0;i<numClusters;++i){
                gridInit[i*2+0] = (uint32_t)(i * m_MaxLightsPerCluster); // offset
                gridInit[i*2+1] = 0u;                                    // count
            }
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, gridInit.size()*sizeof(uint32_t), gridInit.data());
        }

        {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_IndexListSSBO);
            const size_t count = numClusters * size_t(m_MaxLightsPerCluster);
            std::vector<uint32_t> zeros(count, 0u);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, zeros.size()*sizeof(uint32_t), zeros.data());
        }

        glUseProgram(m_Program);

        // SSBO bindings
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_LightsSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_ClustersSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_GridSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_IndexListSSBO);

        glUniformMatrix4fv(glGetUniformLocation(m_Program, "u_View"), 1, GL_FALSE, glm::value_ptr(ctx.m_View));
        glUniformMatrix4fv(glGetUniformLocation(m_Program, "u_Projection"), 1, GL_FALSE, glm::value_ptr(ctx.m_Projection));
        glUniform3i(glGetUniformLocation(m_Program, "u_ClusterDims"), m_ClusterDimX, m_ClusterDimY, m_ClusterDimZ);
        glUniform1i(glGetUniformLocation(m_Program, "u_NumLights"), ctx.m_NumberOfLights);
        glUniform1f(glGetUniformLocation(m_Program, "u_NearPlane"), ctx.m_Near);
        glUniform1f(glGetUniformLocation(m_Program, "u_FarPlane"), ctx.m_Far);

        glDispatchCompute(m_ClusterDimX, m_ClusterDimY, m_ClusterDimZ);

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

        glUseProgram(0);
    }

} // namespace Nova::Renderer::OpenGL