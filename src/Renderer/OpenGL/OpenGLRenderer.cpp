#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Renderer/OpenGL/OpenGLRenderer.hpp"
#include "Renderer/OpenGL/GL_Shader.hpp"
#include "Components/TransformComponent.hpp"
#include "Components/LightComponent.hpp"

using namespace Nova::Components;

namespace Nova::Renderer::OpenGL {

    void OpenGLRenderer::init(Nova::Scene& scene) {
        m_Scene = &scene;

        if (glewInit() != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW\n";
        }

        m_Scene->registry().on_destroy<Nova::Components::MeshComponent>().connect<&OpenGLRenderer::onMeshDestroyed>(this);
        m_Scene->registry().on_construct<Nova::Components::MeshComponent>().connect<&OpenGLRenderer::onMeshConstructed>(this);

        buildViewportFBO(m_W, m_H);
        buildShadowFBO();

        m_ShadowPass   = std::make_unique<GL_ShadowPass>(&m_MeshCache, &m_ShadowFBO, &m_ShadowDepth, &m_ShadowSize);
        m_GeometryPass = std::make_unique<GL_GeometryPass>(&m_MeshCache);
        m_OutlinePass  = std::make_unique<GL_OutlinePass>(&m_MeshCache);
    }

    void OpenGLRenderer::buildViewportFBO(int w,int h){
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

    void OpenGLRenderer::buildShadowFBO(){
        if(m_ShadowFBO){ glDeleteFramebuffers(1,&m_ShadowFBO); glDeleteTextures(1,&m_ShadowDepth);}

        glGenFramebuffers(1,&m_ShadowFBO);
        glGenTextures(1,&m_ShadowDepth);
        glBindTexture(GL_TEXTURE_2D,m_ShadowDepth);
        glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24,m_ShadowSize,m_ShadowSize,0,GL_DEPTH_COMPONENT,GL_FLOAT,nullptr);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_BORDER);
        const float border[4]={1,1,1,1};
        glTexParameterfv(GL_TEXTURE_2D,GL_TEXTURE_BORDER_COLOR,border);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_COMPARE_MODE,GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_COMPARE_FUNC,GL_LEQUAL);

        glBindFramebuffer(GL_FRAMEBUFFER,m_ShadowFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,m_ShadowDepth,0);
        glDrawBuffer(GL_NONE); glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER,0);
    }

    void OpenGLRenderer::updateViewportSize(int w,int h){
        if(w==m_W && h==m_H) return; m_W=w; m_H=h; buildViewportFBO(w,h);
        // keep camera aspect synced
        if(auto camE = m_Scene->getViewportCamera(); camE!=entt::null){
            auto& cam = m_Scene->registry().get<CameraComponent>(camE);
            cam.m_AspectRatio = (float)w/(float)h;
        }
    }

    void OpenGLRenderer::render() {
        // fetch camera
        glm::mat4 view(1.0f), proj(1.0f); glm::vec3 camPos(0.0f);
        if(auto camE = m_Scene->getViewportCamera(); camE!=entt::null){
            auto& cam = m_Scene->registry().get<CameraComponent>(camE);
            view = cam.getViewMatrix(); proj = cam.getProjectionMatrix(); camPos = cam.m_LookFrom;
        }

        // One light (first found)
        bool hasLight = false;
        glm::vec3 Lpos(3, 5, 2), Lcol(1.0f);
        float Lint = 1.0f;
        glm::mat4 lightVP(1.0f);

        m_Scene->forEach<LightComponent, TransformComponent>([&](entt::entity, LightComponent& l, TransformComponent& t) {
            if (hasLight) return;
            hasLight = true;

            Lpos = t.m_Position;
            Lcol = l.m_Color;
            Lint = l.m_Intensity;

            // === Direction de la lumière issue de la ROTATION (degrés) ===
            glm::quat q = glm::quat(glm::radians(t.m_Rotation));
            glm::vec3 Ldir = glm::normalize(q * glm::vec3(0.0f, 0.0f, -1.0f)); // -Z local

            // === View de la lumière alignée avec Ldir ===
            // Cam "placée" à Lpos, regardant Lpos + Ldir
            glm::vec3 target = Lpos + Ldir;

            // up robuste (évite colinéarités)
            glm::vec3 up(0.0f, 1.0f, 0.0f);
            if (std::abs(glm::dot(Ldir, up)) > 0.999f) up = glm::vec3(0,0,1);
            if (std::abs(glm::dot(Ldir, up)) > 0.999f) up = glm::vec3(1,0,0);

            glm::mat4 V = glm::lookAt(Lpos, target, up);

            // === Ortho box : garde tes bornes, ou ajuste selon ta scène ===
            glm::mat4 P = glm::ortho(-20.f, 20.f, -20.f, 20.f, 0.1f, 50.f);

            lightVP = P * V;
        });

        RenderContext ctx; ctx.m_Scene=m_Scene; ctx.m_View=view; ctx.m_Proj=proj; ctx.m_CameraPos=camPos;
        ctx.m_HasLight=hasLight; ctx.m_LightPos=Lpos; ctx.m_LightColor=Lcol; ctx.m_LightIntensity=Lint; ctx.m_LightVP=lightVP;
        ctx.m_FBO=m_FBO; ctx.m_ViewportWidth=m_W; ctx.m_ViewportHeight=m_H; ctx.m_ShadowTex=m_ShadowDepth; ctx.m_ShadowSize=m_ShadowSize; ctx.m_ShadowBias=0.0008f;

        // 1) shadow
        if (hasLight) m_ShadowPass->execute(ctx);

        // 2) geometry + lighting
        m_GeometryPass->execute(ctx);

        // 3) outline of selection
        m_OutlinePass->execute(ctx);
    }

    void OpenGLRenderer::onMeshDestroyed(entt::registry& reg, entt::entity ent) {
        if (auto it = m_MeshCache.find(ent); it != m_MeshCache.end()) {
            glDeleteBuffers  (1, &it->second.m_VBO);
            glDeleteBuffers  (1, &it->second.m_IBO);
            glDeleteVertexArrays(1, &it->second.m_VAO);
            m_MeshCache.erase(it);
        }
    }

    void OpenGLRenderer::onMeshConstructed(entt::registry& reg, entt::entity ent) {
        auto& mesh = reg.get<Components::MeshComponent>(ent);
        GL_MeshBuffers entry = createGLMeshBuffers(mesh);
        m_MeshCache[ent] = entry;
    }

    Renderer::OpenGL::GL_MeshBuffers OpenGLRenderer::createGLMeshBuffers(const MeshComponent& mesh) {
        // Upload vertices/normals + indices, configure VAO
        Renderer::OpenGL::GL_MeshBuffers e{};
        glGenVertexArrays(1, &e.m_VAO);
        glBindVertexArray(e.m_VAO);

        glGenBuffers(1, &e.m_VBO);
        glBindBuffer(GL_ARRAY_BUFFER, e.m_VBO);

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

        glGenBuffers(1, &e.m_IBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, e.m_IBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     mesh.m_Indices.size()*sizeof(unsigned),
                     mesh.m_Indices.data(),
                     GL_STATIC_DRAW);

        glBindVertexArray(0);
        return e;
    }

    void OpenGLRenderer::destroy() {
        // Delete viewport FBO resources
        glDeleteFramebuffers(1, &m_FBO);
        glDeleteTextures(1, &m_ColorTexture);

        // Delete shadow-map resources
        if (m_ShadowFBO) { glDeleteFramebuffers(1, &m_ShadowFBO);  m_ShadowFBO = 0; }

        // viewport
        if (m_DepthStencil) { glDeleteRenderbuffers(1, &m_DepthStencil); m_DepthStencil = 0; }

        // shadow-map
        if (m_ShadowDepth) { glDeleteTextures(1, &m_ShadowDepth); m_ShadowDepth = 0; }
    }
} // namespace Nova::Renderer::OpenGL
