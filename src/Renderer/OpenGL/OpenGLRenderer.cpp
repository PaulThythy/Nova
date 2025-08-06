#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Renderer/OpenGL/OpenGLRenderer.hpp"
#include "Renderer/OpenGL/Shader.hpp"

using namespace Nova::Components;

namespace Nova::Renderer::OpenGL {

    void OpenGLRenderer::init(Nova::Scene& scene) {
        m_Scene = &scene;

        if(glewInit() != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW\n";
        }

        std::string vertexPath   = std::string(SHADER_DIR) + "/vertex.vert";
        std::string fragmentPath = std::string(SHADER_DIR) + "/fragment.frag";
        m_shaderProgram = loadRenderShader(vertexPath, fragmentPath);

        std::string outlineVert = std::string(SHADER_DIR) + "/outline.vert";
        std::string outlineFrag = std::string(SHADER_DIR) + "/outline.frag";
        m_outlineProgram = loadRenderShader(outlineVert, outlineFrag);

        if (m_shaderProgram == 0 || m_outlineProgram == 0) {
            std::cerr << "Failed to load/compile shaders!" << std::endl;
        }

        initFBO(m_ViewportWidth, m_ViewportHeight);
    }

    void OpenGLRenderer::initFBO(int width, int height) {        
        if (m_FBO) {
            glDeleteFramebuffers(1, &m_FBO);
            glDeleteTextures(1, &m_ColorTexture);
            glDeleteRenderbuffers(1, &m_DepthBuffer);
        }

        glGenFramebuffers(1, &m_FBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

        glGenTextures(1, &m_ColorTexture);
        glBindTexture(GL_TEXTURE_2D, m_ColorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorTexture, 0);

        glGenRenderbuffers(1, &m_DepthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, m_DepthBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthBuffer);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "FBO not complete!\n";

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLRenderer::updateViewportSize(int width, int height) {
        if (width != m_ViewportWidth || height != m_ViewportHeight) {
            m_ViewportWidth = width;
            m_ViewportHeight = height;
            initFBO(width, height);

            if (m_Scene) {
                auto camEntity = m_Scene->getViewportCamera();
                if (camEntity != entt::null) {
                    auto& cam = m_Scene->registry().get<CameraComponent>(camEntity);
                    cam.m_AspectRatio = static_cast<float>(width) / static_cast<float>(height);
                }
            }
        }
    }

    void OpenGLRenderer::render() {
        // --- FBO + viewport ---
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
        glViewport(0, 0, m_ViewportWidth, m_ViewportHeight);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // --- Récup cam ---
        glm::mat4 view(1.0f), proj(1.0f);
        glm::vec3 camPos(0.0f);
        auto camEntity = m_Scene->getViewportCamera();
        if (camEntity != entt::null) {
            auto& cam = m_Scene->registry().get<CameraComponent>(camEntity);
            view   = cam.getViewMatrix();
            proj   = cam.getProjectionMatrix();
            camPos = cam.m_LookFrom;
        }

        // --- Récup une lumière simple (première) ---
        glm::vec3 lightPos(5.0f, 7.0f, 5.0f);
        glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
        float     lightIntensity = 1.0f;
        m_Scene->forEach<TransformComponent, LightComponent>([&](entt::entity e, auto& tf, auto& lt){
            // on prend la première et basta
            lightPos       = glm::vec3(tf.GetTransform() * glm::vec4(0,0,0,1));
            lightColor     = lt.m_Color;
            lightIntensity = lt.m_Intensity;
            // break déguisé : on ne change plus après la 1ère
        });

        // =========================================================
        // 1) SCENE PASS (AUCUN ÉCRITURE STENCIL)
        // =========================================================
        glDisable(GL_STENCIL_TEST);

        glUseProgram(m_shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "u_View"),       1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "u_Projection"), 1, GL_FALSE, glm::value_ptr(proj));
        glUniform3fv       (glGetUniformLocation(m_shaderProgram, "u_CameraPos"), 1, glm::value_ptr(camPos));

        glUniform3fv(glGetUniformLocation(m_shaderProgram, "u_LightPos"),       1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(m_shaderProgram, "u_LightColor"),     1, glm::value_ptr(lightColor));
        glUniform1f (glGetUniformLocation(m_shaderProgram, "u_LightIntensity"), lightIntensity);

        m_Scene->forEach<TransformComponent, MeshComponent>([&](entt::entity id, TransformComponent& tf, MeshComponent& mesh){
            const auto* mr = m_Scene->registry().try_get<MeshRendererComponent>(id);
            if (mr && !mr->m_Visible) return;

            // Matériaux (défauts si pas de MR)
            if (mr) {
                glUniform3fv(glGetUniformLocation(m_shaderProgram, "u_BaseColor"),         1, glm::value_ptr(mr->m_BaseColor));
                glUniform1f (glGetUniformLocation(m_shaderProgram, "u_Roughness"),            mr->m_Roughness);
                glUniform1f (glGetUniformLocation(m_shaderProgram, "u_Metallic"),             mr->m_Metallic);
                glUniform3fv(glGetUniformLocation(m_shaderProgram, "u_EmissiveColor"),     1, glm::value_ptr(mr->m_EmissiveColor));
                glUniform1f (glGetUniformLocation(m_shaderProgram, "u_EmissiveStrength"),     mr->m_EmissiveStrength);
            } else {
                glUniform3f (glGetUniformLocation(m_shaderProgram, "u_BaseColor"), 1.0f, 1.0f, 1.0f);
                glUniform1f (glGetUniformLocation(m_shaderProgram, "u_Roughness"), 0.8f);
                glUniform1f (glGetUniformLocation(m_shaderProgram, "u_Metallic"),  0.0f);
                glUniform3f (glGetUniformLocation(m_shaderProgram, "u_EmissiveColor"), 0.0f, 0.0f, 0.0f);
                glUniform1f (glGetUniformLocation(m_shaderProgram, "u_EmissiveStrength"), 0.0f);
            }

            // Wireframe éventuel
            glPolygonMode(GL_FRONT_AND_BACK, (mr && mr->m_Wireframe) ? GL_LINE : GL_FILL);

            GLuint vao = uploadMesh(mesh);
            glm::mat4 model = tf.GetTransform();
            glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "u_Model"), 1, GL_FALSE, glm::value_ptr(model));
            glBindVertexArray(vao);
            glDrawElements(GL_TRIANGLES, (GLsizei)mesh.m_Indices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        });

        // =========================================================
        // 2) MASK PASS (STENCIL POUR L’OBJET SÉLECTIONNÉ SEULEMENT)
        // =========================================================
        if (m_Scene->hasSelection()) {
            entt::entity sel = m_Scene->getSelected();
            auto* tf   = m_Scene->registry().try_get<TransformComponent>(sel);
            auto* mesh = m_Scene->registry().try_get<MeshComponent>(sel);
            if (tf && mesh && !mesh->m_Vertices.empty() && !mesh->m_Indices.empty()) {
                glEnable(GL_STENCIL_TEST);
                glStencilMask(0xFF);
                glStencilFunc(GL_ALWAYS, 1, 0xFF);
                glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

                // On n’écrit PAS la couleur, on veut juste tamponner le stencil
                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                // Et on ne veut pas que le depth test coupe le masque : silhouette complète
                glDisable(GL_DEPTH_TEST);

                glUseProgram(m_shaderProgram);
                glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "u_View"),       1, GL_FALSE, glm::value_ptr(view));
                glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "u_Projection"), 1, GL_FALSE, glm::value_ptr(proj));

                GLuint vao = uploadMesh(*mesh);
                glm::mat4 model = tf->GetTransform();
                glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "u_Model"), 1, GL_FALSE, glm::value_ptr(model));
                glBindVertexArray(vao);
                glDrawElements(GL_TRIANGLES, (GLsizei)mesh->m_Indices.size(), GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);

                // Restore writes
                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                glEnable(GL_DEPTH_TEST);
            }
        }

        // =========================================================
        // 3) OUTLINE PASS (NOTEQUAL stencil, depth off)
        // =========================================================
        if (m_Scene->hasSelection()) {
            entt::entity sel = m_Scene->getSelected();
            auto* tf   = m_Scene->registry().try_get<TransformComponent>(sel);
            auto* mesh = m_Scene->registry().try_get<MeshComponent>(sel);
            if (tf && mesh && !mesh->m_Vertices.empty() && !mesh->m_Indices.empty()) {
                glEnable(GL_STENCIL_TEST);
                glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
                glStencilMask(0x00);          // ne pas modifier le stencil ici
                glDisable(GL_DEPTH_TEST);     // dessiner par-dessus tout

                glUseProgram(m_outlineProgram);
                glUniformMatrix4fv(glGetUniformLocation(m_outlineProgram, "u_View"),       1, GL_FALSE, glm::value_ptr(view));
                glUniformMatrix4fv(glGetUniformLocation(m_outlineProgram, "u_Projection"), 1, GL_FALSE, glm::value_ptr(proj));
                glUniform3f (glGetUniformLocation(m_outlineProgram, "u_OutlineColor"), 1.0f, 0.85f, 0.2f);
                glUniform1f (glGetUniformLocation(m_outlineProgram, "u_OutlineWorld"), 0.02f); // épaisseur *en mètres* monde

                GLuint vao = uploadMesh(*mesh);
                glm::mat4 model = tf->GetTransform();
                glUniformMatrix4fv(glGetUniformLocation(m_outlineProgram, "u_Model"), 1, GL_FALSE, glm::value_ptr(model));
                glBindVertexArray(vao);
                glDrawElements(GL_TRIANGLES, (GLsizei)mesh->m_Indices.size(), GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);

                // Restore stencil/depth
                glStencilMask(0xFF);
                glStencilFunc(GL_ALWAYS, 1, 0xFF);
                glDisable(GL_STENCIL_TEST);
                glEnable(GL_DEPTH_TEST);
            }
        }

        // --- Cleanup ---
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glUseProgram(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }


    GLuint OpenGLRenderer::uploadMesh(const Nova::Components::MeshComponent& mesh) {
        static std::unordered_map<const Nova::Components::MeshComponent*,GLuint> cache;
        auto it = cache.find(&mesh);
        if(it!=cache.end()) return it->second;
        GLuint vao,vbo,ibo;
        glGenVertexArrays(1,&vao);
        glBindVertexArray(vao);
        glGenBuffers(1,&vbo);
        glBindBuffer(GL_ARRAY_BUFFER,vbo);
        struct Vertex{glm::vec3 pos,nrm;};
        std::vector<Vertex> data;
        for(size_t i=0;i<mesh.m_Vertices.size();++i)
            data.push_back({mesh.m_Vertices[i],mesh.m_Normals[i]});
        glBufferData(GL_ARRAY_BUFFER,data.size()*sizeof(Vertex),data.data(),GL_STATIC_DRAW);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,pos));glEnableVertexAttribArray(0);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,nrm));glEnableVertexAttribArray(1);
        glGenBuffers(1,&ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,mesh.m_Indices.size()*sizeof(unsigned),mesh.m_Indices.data(),GL_STATIC_DRAW);
        glBindVertexArray(0);
        cache[&mesh]=vao;
        return vao;
    }

    void OpenGLRenderer::destroy() {
        glDeleteVertexArrays(1, &m_VAO);
        glDeleteBuffers(1, &m_VBO);
        if (m_shaderProgram)  glDeleteProgram(m_shaderProgram);
        if (m_outlineProgram) glDeleteProgram(m_outlineProgram);
    }
} // namespace Nova::Renderer::OpenGL
