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

        BuildViewportFBO(m_W, m_H);
    }

    void GL_Renderer::BuildViewportFBO(int w,int h){
        if(m_FBO){ glDeleteFramebuffers(1,&m_FBO); glDeleteTextures(1,&m_ColorTexture); glDeleteRenderbuffers(1,&m_DepthStencil);}    
        glGenFramebuffers(1,&m_FBO);
        glBindFramebuffer(GL_FRAMEBUFFER,m_FBO);

        glGenTextures(1,&m_ColorTexture);
        glBindTexture(GL_TEXTURE_2D,m_ColorTexture);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,m_ColorTexture,0);

        glGenRenderbuffers(1,&m_DepthStencil);
        glBindRenderbuffer(GL_RENDERBUFFER,m_DepthStencil);
        glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH24_STENCIL8,w,h);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_RENDERBUFFER,m_DepthStencil);

        glBindFramebuffer(GL_FRAMEBUFFER,0);
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

        m_DepthPrePass->Execute(ctx);
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
        // Delete viewport FBO resources
        glDeleteFramebuffers(1, &m_FBO);
        glDeleteTextures(1, &m_ColorTexture);

        // Delete passes
        if (m_DepthPrePass) {
            m_DepthPrePass->Destroy();
            delete m_DepthPrePass;
            m_DepthPrePass = nullptr;
        }

        // viewport
        if (m_DepthStencil) { glDeleteRenderbuffers(1, &m_DepthStencil); m_DepthStencil = 0; }
    }
} // namespace Nova::Renderer::OpenGL
