#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#include "Renderer/OpenGL/RenderPass.hpp"
#include "Renderer/OpenGL/Passes/GeometryPass.hpp"

#include "Renderer/OpenGL/Shader.hpp"

#include "Components/TransformComponent.hpp"
#include "Components/MeshRendererComponent.hpp"
#include "Components/LightComponent.hpp"

using namespace Nova::Components;

namespace Nova::Renderer::OpenGL {

    GeometryPass::GeometryPass(std::unordered_map<entt::entity, GLMeshBuffers>* c) : m_Cache(c) {
        std::string vertexPath = std::string(SHADER_DIR) + "/standard.vert";
        std::string fragmentPath = std::string(SHADER_DIR) + "/standard.frag";
        m_Program = loadRenderShader(vertexPath, fragmentPath);

        if (m_Program == 0) {
            std::cerr << "Failed to load/compile shaders!" << std::endl;
        }
    }

    static inline glm::mat3 normalMatrix(const glm::mat4& M){
        return glm::mat3(glm::transpose(glm::inverse(M)));
    }

    void GeometryPass::execute(const RenderContext& ctx){
        glBindFramebuffer(GL_FRAMEBUFFER, ctx.m_FBO);
        glViewport(0,0,ctx.m_ViewportWidth,ctx.m_ViewportHeight);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.1f,0.1f,0.1f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CW);

        glUseProgram(m_Program);
        const GLint uV  = glGetUniformLocation(m_Program, "u_View");
        const GLint uP  = glGetUniformLocation(m_Program, "u_Projection");
        const GLint uM  = glGetUniformLocation(m_Program, "u_Model");
        const GLint uN  = glGetUniformLocation(m_Program, "u_NormalMatrix");
        const GLint uLVP= glGetUniformLocation(m_Program, "u_LightViewProj");
        const GLint uCam= glGetUniformLocation(m_Program, "u_CameraPos");

        glUniformMatrix4fv(uV,1,GL_FALSE,glm::value_ptr(ctx.m_View));
        glUniformMatrix4fv(uP,1,GL_FALSE,glm::value_ptr(ctx.m_Proj));
        glUniform3fv      (uCam,1,glm::value_ptr(ctx.m_CameraPos));
        glUniformMatrix4fv(uLVP,1,GL_FALSE,glm::value_ptr(ctx.m_LightVP));

        // ------------------------------------------------------------
        // LIGHT BINDING — support Directional / Spot / Point
        // ------------------------------------------------------------

        // Valeurs par défaut (fallback sur ce que le RenderContext fournit déjà)
        glm::vec3  lightPos = ctx.m_LightPos;
        glm::vec3  lightDir = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.3f));
        glm::vec3  lightColor = ctx.m_LightColor;
        float      lightIntensity = ctx.m_LightIntensity;
        int        lightType = 0; // 0=Directional, 1=Spot, 2=Point
        float      lightRange = 10.0f;
        float      spotInnerCos = std::cos(glm::radians(15.0f));
        float      spotOuterCos = std::cos(glm::radians(25.0f));
        bool       lightShadows = ctx.m_HasLight; // compat: ancien flag → utilisé comme défaut

        // On essaie de récupérer la 1ère lumière de la scène (Transform + LightComponent)
        bool found = false;
        ctx.m_Scene->forEach<Nova::Components::TransformComponent, Nova::Components::LightComponent>(
            [&](entt::entity e, Nova::Components::TransformComponent& tf, Nova::Components::LightComponent& li)
            {
                if (found) return; // on prend la première
                found = true;

                // Position + direction (rotations stockées en DEGRÉS)
                lightPos = tf.m_Position;
                glm::quat q = glm::quat(glm::radians(tf.m_Rotation));
                lightDir = glm::normalize(q * glm::vec3(0.0f, 0.0f, -1.0f)); // -Z local

                // Couleur / intensité
                lightColor = li.m_Color;
                lightIntensity = li.m_Intensity;
                lightShadows = li.m_LightShadows;

                // Type + paramètres spécifiques
                switch (li.m_Type) {
                case Nova::Components::LightType::Directional:
                    lightType = 0;
                    break;
                case Nova::Components::LightType::Spot:
                    lightType = 1;
                    lightRange = li.m_Range;
                    spotInnerCos = std::cos(glm::radians(li.m_InnerCone));
                    spotOuterCos = std::cos(glm::radians(li.m_OuterCone));
                    break;
                case Nova::Components::LightType::Point:
                    lightType = 2;
                    lightRange = li.m_Range;
                    break;
                }
            });

        // Envoi des uniforms communs
        glUniform1i(glGetUniformLocation(m_Program, "u_LightType"), lightType);
        glUniform3fv(glGetUniformLocation(m_Program, "u_LightColor"), 1, glm::value_ptr(lightColor));
        glUniform1f(glGetUniformLocation(m_Program, "u_LightIntensity"), lightIntensity);
        glUniform1i(glGetUniformLocation(m_Program, "u_LightShadows"), lightShadows ? 1 : 0);

        // Envoi des uniforms par type
        glUniform3fv(glGetUniformLocation(m_Program, "u_LightPos"), 1, glm::value_ptr(lightPos)); // Spot/Point (ok si non utilisés)
        glUniform3fv(glGetUniformLocation(m_Program, "u_LightDir"), 1, glm::value_ptr(lightDir)); // Directional/Spot
        glUniform1f(glGetUniformLocation(m_Program, "u_LightRange"), lightRange);            // Spot/Point
        glUniform1f(glGetUniformLocation(m_Program, "u_SpotInnerCos"), spotInnerCos);          // Spot
        glUniform1f(glGetUniformLocation(m_Program, "u_SpotOuterCos"), spotOuterCos);          // Spot

        // ------------------------------------------------------------
        // SHADOW MAP — on n’applique les ombres que pour la Directionnelle
        // (conforme au standard.frag que je t’ai donné)
        // ------------------------------------------------------------
        if (lightShadows && lightType == 0) {
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, ctx.m_ShadowTex);
            glUniform1i(glGetUniformLocation(m_Program, "u_ShadowMap"), 5);

            // NB: dans le nouveau standard.frag, u_ShadowTexelSize est un vec2
            const float ts = 1.0f / float(ctx.m_ShadowSize);
            glUniform2f(glGetUniformLocation(m_Program, "u_ShadowTexelSize"), ts, ts);
        }

        ctx.m_Scene->forEach<TransformComponent, MeshComponent>([&](entt::entity e, TransformComponent& tf, MeshComponent& mesh){
            const auto* mr = ctx.m_Scene->registry().try_get<MeshRendererComponent>(e);
            if(mr && !mr->m_Visible) return; if(mesh.m_Indices.empty()) return;

            auto it = m_Cache->find(e); if(it==m_Cache->end()) return;

            glm::mat4 M = tf.GetTransform(); glm::mat3 N = normalMatrix(M);
            glUniformMatrix4fv(uM,1,GL_FALSE,glm::value_ptr(M));
            glUniformMatrix3fv(uN,1,GL_FALSE,glm::value_ptr(N));

            // material
            if(mr){
                glUniform3fv(glGetUniformLocation(m_Program,"u_BaseColor"),1,glm::value_ptr(mr->m_BaseColor));
                glUniform1f (glGetUniformLocation(m_Program,"u_Roughness"), mr->m_Roughness);
                glUniform1f (glGetUniformLocation(m_Program,"u_Metallic" ), mr->m_Metallic);
                glUniform3fv(glGetUniformLocation(m_Program,"u_EmissiveColor"),1,glm::value_ptr(mr->m_EmissiveColor));
                glUniform1f (glGetUniformLocation(m_Program,"u_EmissiveStrength"), mr->m_EmissiveStrength);
            } else {
                glUniform3f(glGetUniformLocation(m_Program,"u_BaseColor"),1,1,1);
                glUniform1f(glGetUniformLocation(m_Program,"u_Roughness"),0.8f);
                glUniform1f(glGetUniformLocation(m_Program,"u_Metallic" ),0.0f);
                glUniform3f(glGetUniformLocation(m_Program,"u_EmissiveColor"),0,0,0);
                glUniform1f(glGetUniformLocation(m_Program,"u_EmissiveStrength"),0.0f);
            }

            // wireframe?
            if(mr && mr->m_Wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

            glBindVertexArray(it->second.m_VAO);
            glDrawElements(GL_TRIANGLES,(GLsizei)mesh.m_Indices.size(),GL_UNSIGNED_INT,0);
            glBindVertexArray(0);

            if(mr && mr->m_Wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        });

        glUseProgram(0);
        glBindFramebuffer(GL_FRAMEBUFFER,0);

        glDisable(GL_CULL_FACE);
    }

} // Nova::Renderer::OpenGL