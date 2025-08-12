#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Renderer/OpenGL/OpenGLRenderer.hpp"
#include "Renderer/OpenGL/Shader.hpp"

using namespace Nova::Components;

namespace Nova::Renderer::OpenGL {

    void OpenGLRenderer::init(Nova::Scene& scene) {
        m_Scene = &scene;

        m_Scene->registry().on_destroy<Nova::Components::MeshComponent>().connect<&OpenGLRenderer::onMeshDestroyed>(this);
        m_Scene->registry().on_construct<Nova::Components::MeshComponent>().connect<&OpenGLRenderer::onMeshConstructed>(this);

        if(glewInit() != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW\n";
        }

        // Main forward shading program
        std::string vertexPath   = std::string(SHADER_DIR) + "/vertex.vert";
        std::string fragmentPath = std::string(SHADER_DIR) + "/fragment.frag";
        m_ShaderProgram = loadRenderShader(vertexPath, fragmentPath);

        // Outline shader (stencil-based highlight)
        std::string outlineVert = std::string(SHADER_DIR) + "/outline.vert";
        std::string outlineFrag = std::string(SHADER_DIR) + "/outline.frag";
        m_OutlineProgram = loadRenderShader(outlineVert, outlineFrag);

        // Depth-only shader for shadow-map generation
        std::string shadowVert = std::string(SHADER_DIR) + "/shadowDepth.vert";
        std::string shadowFrag = std::string(SHADER_DIR) + "/shadowDepth.frag";
        m_ShadowProgram = loadRenderShader(shadowVert, shadowFrag);

        if (m_ShaderProgram == 0 || m_OutlineProgram == 0 || m_ShadowProgram == 0) {
            std::cerr << "Failed to load/compile shaders!" << std::endl;
        }

        // Render target for the viewport panel
        initFBO(m_ViewportWidth, m_ViewportHeight);
        // Shadow-map framebuffer/texture
        initShadows();
    }

    void OpenGLRenderer::initFBO(int width, int height) {        
        if (m_FBO) {
            glDeleteFramebuffers(1, &m_FBO);
            glDeleteTextures(1, &m_ColorTexture);
            glDeleteRenderbuffers(1, &m_DepthBuffer);
        }

        glGenFramebuffers(1, &m_FBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

        // Color attachment (RGBA8)
        glGenTextures(1, &m_ColorTexture);
        glBindTexture(GL_TEXTURE_2D, m_ColorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorTexture, 0);

        // Depth + stencil (combined renderbuffer)
        glGenRenderbuffers(1, &m_DepthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, m_DepthBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthBuffer);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "FBO not complete!\n";

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLRenderer::initShadows() {
        glGenFramebuffers(1, &m_ShadowFBO);
        glGenTextures(1, &m_ShadowDepthTex);
        glBindTexture(GL_TEXTURE_2D, m_ShadowDepthTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
            m_ShadowSize, m_ShadowSize, 0,
            GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float border[4] = { 1,1,1,1 };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
        // Enable hardware depth compare for sampler2DShadow sampling
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

        glBindFramebuffer(GL_FRAMEBUFFER, m_ShadowFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_ShadowDepthTex, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "Shadow FBO not complete!\n";
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLRenderer::updateViewportSize(int width, int height) {
        if (width != m_ViewportWidth || height != m_ViewportHeight) {
            m_ViewportWidth = width;
            m_ViewportHeight = height;
            initFBO(width, height);

            // Keep the camera aspect-ratio in sync with the viewport
            if (m_Scene) {
                auto camEntity = m_Scene->getViewportCamera();
                if (camEntity != entt::null) {
                    auto& cam = m_Scene->registry().get<CameraComponent>(camEntity);
                    cam.m_AspectRatio = static_cast<float>(width) / static_cast<float>(height);
                }
            }
        }
    }

    void OpenGLRenderer::renderShadowPass(const glm::mat4& lightVP) {
        // Render all shadow-casting meshes into the depth texture (light's POV)
        m_LastLightVP = lightVP;

        glViewport(0, 0, m_ShadowSize, m_ShadowSize);
        glBindFramebuffer(GL_FRAMEBUFFER, m_ShadowFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT); // reduce shadow acne (front-face culling)

        glUseProgram(m_ShadowProgram);
        GLint loc_LVP = glGetUniformLocation(m_ShadowProgram, "u_LightViewProj");
        GLint loc_M = glGetUniformLocation(m_ShadowProgram, "u_Model");

        glUniformMatrix4fv(loc_LVP, 1, GL_FALSE, glm::value_ptr(lightVP));

        // Draw all meshes that cast shadows
        m_Scene->forEach<TransformComponent, MeshComponent>([&](entt::entity id, TransformComponent& tf, MeshComponent& mesh) {
            const auto* mr = m_Scene->registry().try_get<MeshRendererComponent>(id);
            if (mr && !mr->m_CastShadows) return;
            auto it = m_MeshCache.find(id);
            if (it == m_MeshCache.end()) return;

            glm::mat4 model = tf.GetTransform();
            glUniformMatrix4fv(loc_M, 1, GL_FALSE, glm::value_ptr(model));
            glBindVertexArray(it->second.m_VAO);
            glDrawElements(GL_TRIANGLES, (GLsizei)mesh.m_Indices.size(), GL_UNSIGNED_INT, 0);
            });

        // Leave depth culling in a sane state for subsequent passes
        glCullFace(GL_BACK);
        glBindVertexArray(0);
        glUseProgram(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLRenderer::render() {
        // Bind the viewport render target and clear
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
        glViewport(0, 0, m_ViewportWidth, m_ViewportHeight);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // Fetch active camera
        glm::mat4 view(1.0f), proj(1.0f);
        glm::vec3 camPos(0.0f);
        auto camEntity = m_Scene->getViewportCamera();
        if (camEntity != entt::null) {
            auto& cam = m_Scene->registry().get<CameraComponent>(camEntity);
            view   = cam.getViewMatrix();
            proj   = cam.getProjectionMatrix();
            camPos = cam.m_LookFrom;
        }

        glDisable(GL_STENCIL_TEST);

        // ---------------------------------------------------------
        // Gather all lights and build a light view-projection (VP)
        // for each one, then generate a shadow map.
        // ---------------------------------------------------------
        struct LightData { glm::vec3 pos, color; float intensity; glm::mat4 lightVP; };
        std::vector<LightData> lights;
        m_Scene->forEach<TransformComponent, LightComponent>([&](entt::entity e, auto& tf, auto& lt) {
            LightData L{};
            L.pos = glm::vec3(tf.GetTransform() * glm::vec4(0, 0, 0, 1));
            L.color = lt.m_Color;
            L.intensity = lt.m_Intensity;

            // Simple directional-like VP: look from light toward an approximate scene center
            glm::vec3 center(0.0f);
            int count = 0;
            m_Scene->forEach<TransformComponent>([&](entt::entity, TransformComponent& t) {
                center += t.m_Position; ++count;
                });
            if (count > 0) center /= float(count);
            glm::vec3 up(0, 1, 0);
            glm::mat4 lightView = glm::lookAt(L.pos, center, up);

            float s = 15.0f; // ortho frustum size (TODO: expose)
            glm::mat4 lightProj = glm::ortho(-s, s, -s, s, 0.1f, 60.0f);
            L.lightVP = lightProj * lightView;

            // Generate the shadow map for this light
            renderShadowPass(L.lightVP);

            lights.push_back(L);
        });

        // Restore main render target / viewport after shadow pass(es)
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
        glViewport(0, 0, m_ViewportWidth, m_ViewportHeight);
        glDisable(GL_CULL_FACE);


        // ---------------------------------------------------------
        // 1) Scene pass (no stencil writes)
        //    - If no lights, consider adding a simple fallback pass.
        // ---------------------------------------------------------
        if (lights.empty()) {
            glUseProgram(m_ShaderProgram);
            glUniformMatrix4fv(glGetUniformLocation(m_ShaderProgram, "u_View"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(m_ShaderProgram, "u_Projection"), 1, GL_FALSE, glm::value_ptr(proj));
            glUniform3fv(glGetUniformLocation(m_ShaderProgram, "u_CameraPos"), 1, glm::value_ptr(camPos));
            glUniform1i(glGetUniformLocation(m_ShaderProgram, "u_AdditivePass"), 2);
            glUniform1i(glGetUniformLocation(m_ShaderProgram, "u_UseShadowMap"), 0);
        }
        // With lights: one pass per light, additive blending to accumulate lighting
        bool first = true;
        for (const auto& L : lights) {
            if (!first) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_ONE, GL_ONE);   // additive accumulation
                glDepthMask(GL_FALSE);         // do not overwrite depth on subsequent light passes
            }
            else {
                glDisable(GL_BLEND);
                glDepthMask(GL_TRUE);
            }

            glUseProgram(m_ShaderProgram);
            glUniformMatrix4fv(glGetUniformLocation(m_ShaderProgram, "u_View"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(m_ShaderProgram, "u_Projection"), 1, GL_FALSE, glm::value_ptr(proj));
            glUniform3fv(glGetUniformLocation(m_ShaderProgram, "u_CameraPos"), 1, glm::value_ptr(camPos));

            glUniform3fv(glGetUniformLocation(m_ShaderProgram, "u_LightPos"), 1, glm::value_ptr(L.pos));
            glUniform3fv(glGetUniformLocation(m_ShaderProgram, "u_LightColor"), 1, glm::value_ptr(L.color));
            glUniform1f(glGetUniformLocation(m_ShaderProgram, "u_LightIntensity"), L.intensity);

            // Shadow uniforms
            glUniform1i(glGetUniformLocation(m_ShaderProgram, "u_AdditivePass"), first ? 0 : 1);
            glUniform1i(glGetUniformLocation(m_ShaderProgram, "u_UseShadowMap"), 1);
            glUniform1f(glGetUniformLocation(m_ShaderProgram, "u_ShadowBias"), 0.0008f);
            glUniform1f(glGetUniformLocation(m_ShaderProgram, "u_ShadowTexelSize"), 1.0f / float(m_ShadowSize));
            glUniformMatrix4fv(glGetUniformLocation(m_ShaderProgram, "u_LightViewProj"), 1, GL_FALSE, glm::value_ptr(L.lightVP));

            // Bind shadow map on texture unit 5
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, m_ShadowDepthTex);
            glUniform1i(glGetUniformLocation(m_ShaderProgram, "u_ShadowMap"), 5);

            // Draw all visible meshes (same routine as the previous single-pass version)
            m_Scene->forEach<TransformComponent, MeshComponent>([&](entt::entity id, TransformComponent& tf, MeshComponent& mesh) {
                const auto* mr = m_Scene->registry().try_get<MeshRendererComponent>(id);
                if (mr && !mr->m_Visible) return;

                // Material parameters (defaults if no MeshRendererComponent)
                if (mr) {
                    glUniform3fv(glGetUniformLocation(m_ShaderProgram, "u_BaseColor"), 1, glm::value_ptr(mr->m_BaseColor));
                    glUniform1f(glGetUniformLocation(m_ShaderProgram, "u_Roughness"), mr->m_Roughness);
                    glUniform1f(glGetUniformLocation(m_ShaderProgram, "u_Metallic"), mr->m_Metallic);
                    glUniform3fv(glGetUniformLocation(m_ShaderProgram, "u_EmissiveColor"), 1, glm::value_ptr(mr->m_EmissiveColor));
                    glUniform1f(glGetUniformLocation(m_ShaderProgram, "u_EmissiveStrength"), mr->m_EmissiveStrength);
                }
                else {
                    glUniform3f(glGetUniformLocation(m_ShaderProgram, "u_BaseColor"), 1.0f, 1.0f, 1.0f);
                    glUniform1f(glGetUniformLocation(m_ShaderProgram, "u_Roughness"), 0.8f);
                    glUniform1f(glGetUniformLocation(m_ShaderProgram, "u_Metallic"), 0.0f);
                    glUniform3f(glGetUniformLocation(m_ShaderProgram, "u_EmissiveColor"), 0.0f, 0.0f, 0.0f);
                    glUniform1f(glGetUniformLocation(m_ShaderProgram, "u_EmissiveStrength"), 0.0f);
                }

                // Optional wireframe
                glPolygonMode(GL_FRONT_AND_BACK, (mr && mr->m_Wireframe) ? GL_LINE : GL_FILL);

                auto it = m_MeshCache.find(id);
                if (it == m_MeshCache.end()) return;

                glm::mat4 model = tf.GetTransform();
                glUniformMatrix4fv(glGetUniformLocation(m_ShaderProgram, "u_Model"), 1, GL_FALSE, glm::value_ptr(model));
                // Normal matrix for correct lighting
                glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(model)));
                glUniformMatrix3fv(glGetUniformLocation(m_ShaderProgram, "u_NormalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMat));

                glBindVertexArray(it->second.m_VAO);
                glDrawElements(GL_TRIANGLES, (GLsizei)mesh.m_Indices.size(), GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
                });
            // ------------------------------------------------------------------

            first = false;
        }
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        // ---------------------------------------------------------
        // 2) MASK pass (stencil for selected object only)
        //    - We stamp the stencil with the selected object's silhouette
        //      (color writes OFF, depth test OFF)
        // ---------------------------------------------------------
        if (m_Scene->hasSelection()) {
            for (entt::entity sel: m_Scene->getSelected()) {
                auto* tf   = m_Scene->registry().try_get<TransformComponent>(sel);
                auto* mesh = m_Scene->registry().try_get<MeshComponent>(sel);
                if (!tf || !mesh || mesh->m_Vertices.empty() || mesh->m_Indices.empty()) continue;

                auto it = m_MeshCache.find(sel);
                if (it == m_MeshCache.end()) {
                    // Option A: lazy build (if desired)
                    // it = m_MeshCache.emplace(sel, createGLMeshBuffers(*mesh)).first;
                    continue;
                }
                const GLuint vao = it->second.m_VAO;

                glEnable(GL_STENCIL_TEST);
                glStencilMask(0xFF);
                glStencilFunc(GL_ALWAYS, 1, 0xFF);
                glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

                // We only write to stencil, not to color
                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                // Disable depth test so the full silhouette is written
                glDisable(GL_DEPTH_TEST);

                glUseProgram(m_ShaderProgram);
                glUniformMatrix4fv(glGetUniformLocation(m_ShaderProgram, "u_View"),       1, GL_FALSE, glm::value_ptr(view));
                glUniformMatrix4fv(glGetUniformLocation(m_ShaderProgram, "u_Projection"), 1, GL_FALSE, glm::value_ptr(proj));

                glm::mat4 model = tf->GetTransform();
                glUniformMatrix4fv(glGetUniformLocation(m_ShaderProgram, "u_Model"), 1, GL_FALSE, glm::value_ptr(model));
                glBindVertexArray(vao);
                glDrawElements(GL_TRIANGLES, (GLsizei)mesh->m_Indices.size(), GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);

                // Restore color/depth writes
                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                glEnable(GL_DEPTH_TEST);
            }
        }

        // ---------------------------------------------------------
        // 3) OUTLINE pass (draw where stencil != 1, depth test OFF)
        // ---------------------------------------------------------
        if (m_Scene->hasSelection()) {
            for (entt::entity sel : m_Scene->getSelected()) {
                auto* tf   = m_Scene->registry().try_get<TransformComponent>(sel);
                auto* mesh = m_Scene->registry().try_get<MeshComponent>(sel);
                if (!tf || !mesh || mesh->m_Vertices.empty() || mesh->m_Indices.empty()) continue;

                auto it = m_MeshCache.find(sel);
                if (it == m_MeshCache.end()) {
                    // Option A: lazy build (if desired)
                    // it = m_MeshCache.emplace(sel, createGLMeshBuffers(*mesh)).first;
                    continue;
                }
                const GLuint vao = it->second.m_VAO;

                glEnable(GL_STENCIL_TEST);
                glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
                glStencilMask(0x00);          // don't modify stencil here
                glDisable(GL_DEPTH_TEST);     // draw on top of everything

                glUseProgram(m_OutlineProgram);
                glUniformMatrix4fv(glGetUniformLocation(m_OutlineProgram, "u_View"),       1, GL_FALSE, glm::value_ptr(view));
                glUniformMatrix4fv(glGetUniformLocation(m_OutlineProgram, "u_Projection"), 1, GL_FALSE, glm::value_ptr(proj));
                glUniform3f (glGetUniformLocation(m_OutlineProgram, "u_OutlineColor"), 1.0f, 0.85f, 0.2f);
                glUniform1f (glGetUniformLocation(m_OutlineProgram, "u_OutlineWorld"), 0.02f); // outline thickness in world meters

                glm::mat4 model = tf->GetTransform();
                glUniformMatrix4fv(glGetUniformLocation(m_OutlineProgram, "u_Model"), 1, GL_FALSE, glm::value_ptr(model));
                glBindVertexArray(vao);
                glDrawElements(GL_TRIANGLES, (GLsizei)mesh->m_Indices.size(), GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);

                // Restore stencil/depth state
                glStencilMask(0xFF);
                glStencilFunc(GL_ALWAYS, 1, 0xFF);
                glDisable(GL_STENCIL_TEST);
                glEnable(GL_DEPTH_TEST);
            }
        }

        // Final state cleanup for the frame
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glUseProgram(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
        GLMeshBuffers entry = createGLMeshBuffers(mesh);
        m_MeshCache[ent] = entry;
    }

    OpenGLRenderer::GLMeshBuffers OpenGLRenderer::createGLMeshBuffers(const Nova::Components::MeshComponent& mesh) {
        // Upload vertices/normals + indices, configure VAO
        OpenGLRenderer::GLMeshBuffers e{};
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
        // Delete shader programs
        if (m_ShaderProgram)  glDeleteProgram(m_ShaderProgram);
        if (m_OutlineProgram) glDeleteProgram(m_OutlineProgram);

        // Delete viewport FBO resources
        glDeleteFramebuffers(1, &m_FBO);
        glDeleteTextures(1, &m_ColorTexture);
        glDeleteRenderbuffers(1, &m_DepthBuffer);

        // Delete shadow-map resources
        if (m_ShadowProgram) glDeleteProgram(m_ShadowProgram);
        if (m_ShadowDepthTex) { glDeleteTextures(1, &m_ShadowDepthTex); m_ShadowDepthTex = 0; }
        if (m_ShadowFBO) { glDeleteFramebuffers(1, &m_ShadowFBO);  m_ShadowFBO = 0; }
    }
} // namespace Nova::Renderer::OpenGL
