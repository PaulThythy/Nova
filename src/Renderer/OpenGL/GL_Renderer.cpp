#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Renderer/OpenGL/GL_Renderer.hpp"
#include "Renderer/OpenGL/GL_Shader.hpp"
#include "Components/TransformComponent.hpp"
#include "Components/LightComponent.hpp"

using namespace Nova::Components;

namespace Nova::Renderer::OpenGL {

    void GL_Renderer::Init(Nova::Scene& scene) {
        m_Scene = &scene;

        if (glewInit() != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW\n";
        }

        m_Scene->Registry().on_destroy<Nova::Components::MeshComponent>().connect<&GL_Renderer::OnMeshDestroyed>(this);
        m_Scene->Registry().on_construct<Nova::Components::MeshComponent>().connect<&GL_Renderer::OnMeshCreated>(this);

        //m_Scene->Registry().on_construct<Nova::Components::LightComponent>().connect<&GL_Renderer::OnLightCreated>(this);
        //m_Scene->Registry().on_destroy<Nova::Components::LightComponent>().connect<&GL_Renderer::OnLightDestroyed>(this);

        //passes
        m_DepthPrePass = new GL_DepthPrePass();
        m_DepthPrePass->Init();

        m_GBufferPass = new GL_GBufferPass();
        m_GBufferPass->Init();

        m_LightCullingPass = new GL_LightCullingPass();
        m_LightCullingPass->Init();

        BuildViewportFBO(m_W, m_H);
    }

    void GL_Renderer::BuildViewportFBO(int w,int h){
        if (m_FBO) {
            glDeleteFramebuffers(1, &m_FBO);
            if (m_GPosition)        glDeleteTextures(1, &m_GPosition);
            if (m_GNormal)          glDeleteTextures(1, &m_GNormal);
            if (m_GAlbedoRoughness) glDeleteTextures(1, &m_GAlbedoRoughness);
            if (m_GMetallic)        glDeleteTextures(1, &m_GMetallic);
            if (m_DepthTex)         glDeleteTextures(1, &m_DepthTex);
            m_FBO = m_GPosition = m_GNormal = m_GAlbedoRoughness = m_GMetallic = m_DepthTex = 0;
        }

        glGenFramebuffers(1, &m_FBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

        // G-Position (RGBA16F pour world pos + padding)
        glGenTextures(1, &m_GPosition);
        glBindTexture(GL_TEXTURE_2D, m_GPosition);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_GPosition, 0);

        // G-Normal (RGBA16F pour normale monde encodée)
        glGenTextures(1, &m_GNormal);
        glBindTexture(GL_TEXTURE_2D, m_GNormal);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_GNormal, 0);

        // G-Albedo+Roughness (RGBA8: albedo RGB8, roughness A8)
        glGenTextures(1, &m_GAlbedoRoughness);
        glBindTexture(GL_TEXTURE_2D, m_GAlbedoRoughness);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, m_GAlbedoRoughness, 0);

        // G-Metallic
        glGenTextures(1, &m_GMetallic);
        glBindTexture(GL_TEXTURE_2D, m_GMetallic);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, m_GMetallic, 0);
        
        GLint swizzle[4] = { GL_RED, GL_RED, GL_RED, GL_ONE };
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);

        // Depth (texture)
        glGenTextures(1, &m_DepthTex);
        glBindTexture(GL_TEXTURE_2D, m_DepthTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_DepthTex, 0);

        // target list
        GLenum bufs[4] = {
            GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
            GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3
        };
        glDrawBuffers(4, bufs);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
            std::cerr << "Viewport FBO is not complete!\n";
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GL_Renderer::UpdateViewportSize(int w,int h){
        if(w==m_W && h==m_H) return; m_W=w; m_H=h; BuildViewportFBO(w,h);
        // keep camera aspect synced
        if(auto camE = m_Scene->GetViewportCamera(); camE!=entt::null){
            auto& cam = m_Scene->Registry().get<CameraComponent>(camE);
            cam.m_AspectRatio = (float)w/(float)h;
        }
    }

    void GL_Renderer::Render() {
        GL_RenderPassCtx ctx;
        ctx.m_Scene = m_Scene;
        ctx.m_FBO   = m_FBO;
        ctx.m_Width = m_W;
        ctx.m_Height= m_H;

        if (auto camE = m_Scene->GetViewportCamera(); camE != entt::null) {
            auto& reg = m_Scene->Registry();
            auto& cam = reg.get<CameraComponent>(camE);

            ctx.m_View = cam.GetViewMatrix();
            ctx.m_Projection = cam.GetProjectionMatrix();
            ctx.m_Near = cam.m_NearPlane;
            ctx.m_Far  = cam.m_FarPlane;
        }
        // gather lights
        ctx.m_Lights.clear();
        auto& reg = m_Scene->Registry();
        auto viewLights = reg.view<Components::TransformComponent, Components::LightComponent>();
        ctx.m_Lights.reserve(viewLights.size_hint());

        viewLights.each([&](auto ent, const auto& tc, const auto& lc) {
            const auto& transformComp = viewLights.get<Components::TransformComponent>(ent);
            const auto& lightComp = viewLights.get<Components::LightComponent>(ent);

            GL_RenderPassCtx::GPULight L{};
            L.m_Position     = transformComp.m_Position;            // world-space
            L.m_Direction    = transformComp.m_Rotation;            // world-space
            L.m_Color        = lightComp.m_Color;
            L.m_Intensity    = lightComp.m_Intensity;
            L.m_LightShadows = lightComp.m_LightShadows ? 1 : 0;
            // type
            int type = 1; // default point
            switch (lightComp.m_Type) {
                case Components::LightType::Directional: type = 0; break;
                case Components::LightType::Point:       type = 1; break;
                case Components::LightType::Spot:        type = 2; break;
            }
            L.m_Type = type;
            L.m_Range    = lightComp.m_Range;
            L.m_InnerCos = glm::cos(glm::radians(lightComp.m_InnerCone));
            L.m_OuterCos = glm::cos(glm::radians(lightComp.m_OuterCone));

            ctx.m_Lights.push_back(L);
        });
        ctx.m_NumberOfLights = (int)ctx.m_Lights.size();

        m_DepthPrePass->Execute(ctx);
        m_GBufferPass->Execute(ctx);
        m_LightCullingPass->Execute(ctx);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GL_Renderer::OnLightCreated(entt::registry& reg, entt::entity ent) {
        auto& light = reg.get<Components::LightComponent>(ent);
        GL_LightBuffers entry;
        glGenBuffers(1, &entry.m_BufferID);
        glBindBuffer(GL_UNIFORM_BUFFER, entry.m_BufferID);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(light), &light, GL_STATIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        m_LightCache[ent] = entry;
    }

    void GL_Renderer::OnLightDestroyed(entt::registry& reg, entt::entity ent) {
        if (auto it = m_LightCache.find(ent); it != m_LightCache.end()) {
            glDeleteBuffers(1, &it->second.m_BufferID);
            m_LightCache.erase(it);
        }
    }

    void GL_Renderer::OnMeshDestroyed(entt::registry& reg, entt::entity ent) {
        if (auto it = m_MeshCache.find(ent); it != m_MeshCache.end()) {
            glDeleteBuffers  (1, &it->second.m_VBO);
            glDeleteBuffers  (1, &it->second.m_IBO);
            glDeleteVertexArrays(1, &it->second.m_VAO);
            m_MeshCache.erase(it);
        }
    }

    void GL_Renderer::OnMeshCreated(entt::registry& reg, entt::entity ent) {
        auto& mesh = reg.get<Components::MeshComponent>(ent);
        GL_MeshBuffers entry;

        glGenVertexArrays(1, &entry.m_VAO);
        glBindVertexArray(entry.m_VAO);

        glGenBuffers(1, &entry.m_VBO);
        glBindBuffer(GL_ARRAY_BUFFER, entry.m_VBO);

        //TODO vertex struct in separated file
        struct Vertex { glm::vec3 pos, nrm; };
        std::vector<Vertex> data;
        data.reserve(mesh.m_Vertices.size());
        for (size_t i = 0; i < mesh.m_Vertices.size(); ++i)
            data.push_back({mesh.m_Vertices[i], mesh.m_Normals[i]});
        
        glBufferData(GL_ARRAY_BUFFER, data.size()*sizeof(Vertex), data.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex,pos));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex,nrm));
        glEnableVertexAttribArray(1);

        glGenBuffers(1, &entry.m_IBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, entry.m_IBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.m_Indices.size()*sizeof(unsigned), mesh.m_Indices.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);

        m_MeshCache[ent] = entry;
    }

    void GL_Renderer::Destroy() {
        if (m_FBO) {
            glDeleteFramebuffers(1, &m_FBO);
            m_FBO = 0;
        }
        if (m_GPosition)        { glDeleteTextures(1, &m_GPosition);        m_GPosition = 0; }
        if (m_GNormal)          { glDeleteTextures(1, &m_GNormal);          m_GNormal = 0; }
        if (m_GAlbedoRoughness) { glDeleteTextures(1, &m_GAlbedoRoughness); m_GAlbedoRoughness = 0; }
        if (m_GMetallic)        { glDeleteTextures(1, &m_GMetallic);        m_GMetallic = 0; }
        if (m_DepthTex)         { glDeleteTextures(1, &m_DepthTex);         m_DepthTex = 0; }
    }
} // namespace Nova::Renderer::OpenGL
