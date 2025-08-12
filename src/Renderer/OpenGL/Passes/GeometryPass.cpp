#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#include "Renderer/OpenGL/RenderPass.hpp"
#include "Renderer/OpenGL/Passes/GeometryPass.hpp"

#include "Renderer/OpenGL/Shader.hpp"

#include "Components/TransformComponent.hpp"
#include "Components/MeshRendererComponent.hpp"

using namespace Nova::Components;

namespace Nova::Renderer::OpenGL {

    GeometryPass::GeometryPass(std::unordered_map<entt::entity, GLMeshBuffers>* c) : m_Cache(c) {
        std::string vertexPath = std::string(SHADER_DIR) + "/vertex.vert";
        std::string fragmentPath = std::string(SHADER_DIR) + "/fragment.frag";
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

        // light
        glUniform3fv(glGetUniformLocation(m_Program,"u_LightPos"),1,glm::value_ptr(ctx.m_LightPos));
        glUniform3fv(glGetUniformLocation(m_Program,"u_LightColor"),1,glm::value_ptr(ctx.m_LightColor));
        glUniform1f (glGetUniformLocation(m_Program,"u_LightIntensity"), ctx.m_LightIntensity);

        // shadow samplers
        glUniform1i(glGetUniformLocation(m_Program,"u_UseShadowMap"), ctx.m_HasLight ? 1 : 0);
        glUniform1f(glGetUniformLocation(m_Program,"u_ShadowBias"), ctx.m_ShadowBias);
        glUniform1f(glGetUniformLocation(m_Program,"u_ShadowTexelSize"), 1.0f/float(ctx.m_ShadowSize));
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, ctx.m_ShadowTex);
        glUniform1i(glGetUniformLocation(m_Program,"u_ShadowMap"),5);

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