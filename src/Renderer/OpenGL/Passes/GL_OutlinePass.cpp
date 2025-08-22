#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#include "Renderer/OpenGL/GL_RenderPass.hpp"
#include "Renderer/OpenGL/Passes/GL_OutlinePass.hpp"

#include "Renderer/OpenGL/GL_Shader.hpp"

using namespace Nova::Components;

namespace Nova::Renderer::OpenGL {

    GL_OutlinePass::GL_OutlinePass(std::unordered_map<entt::entity, GL_MeshBuffers>* c) : m_Cache(c) {
        std::string vertexPath = std::string(SHADER_DIR) + "/outline.vert";
        std::string fragmentPath = std::string(SHADER_DIR) + "/outline.frag";
        m_OutlineProgram = loadRenderShader(vertexPath, fragmentPath);

        if (m_OutlineProgram == 0) {
            std::cerr << "Failed to load/compile shaders!" << std::endl;
        }
    }

    void GL_OutlinePass::execute(const RenderContext& ctx){
        if (!ctx.m_Scene || !ctx.m_Scene->hasSelection() || m_OutlineProgram == 0)
            return;

        // 2.1 write stencil with selected geometry (no color, no depth)
        glBindFramebuffer(GL_FRAMEBUFFER, ctx.m_FBO);
        glViewport(0, 0, ctx.m_ViewportWidth, ctx.m_ViewportHeight);

        glEnable(GL_STENCIL_TEST);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glStencilMask(0xFF);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        glEnable(GL_DEPTH_TEST);
        glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
        glDisable(GL_DEPTH_TEST);

        glUseProgram(m_OutlineProgram);
        glUniformMatrix4fv(glGetUniformLocation(m_OutlineProgram,"u_View"),1,GL_FALSE,glm::value_ptr(ctx.m_View));
        glUniformMatrix4fv(glGetUniformLocation(m_OutlineProgram,"u_Projection"),1,GL_FALSE,glm::value_ptr(ctx.m_Proj));
        glUniform1f(glGetUniformLocation(m_OutlineProgram, "u_OutlineWorld"), 0.0f);

        for(auto e: ctx.m_Scene->getSelected()){
            auto* tf = ctx.m_Scene->registry().try_get<TransformComponent>(e);
            auto* me = ctx.m_Scene->registry().try_get<MeshComponent>(e);
            if(!tf||!me||me->m_Indices.empty()) continue;
            auto it = m_Cache->find(e); if(it==m_Cache->end()) continue;
            glm::mat4 M = tf->GetTransform();

            glUniformMatrix4fv(glGetUniformLocation(m_OutlineProgram,"u_Model"),1,GL_FALSE,glm::value_ptr(M));
            glBindVertexArray(it->second.m_VAO);
            glDrawElements(GL_TRIANGLES,(GLsizei)me->m_Indices.size(),GL_UNSIGNED_INT,0);
        }

        // 2.2 draw expanded copy wherever stencil==1
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilMask(0x00); 

        glUseProgram(m_OutlineProgram);
        glUniform3f(glGetUniformLocation(m_OutlineProgram,"u_OutlineColor"), 1.0f,1.0f,0.0f);
        glUniform1f(glGetUniformLocation(m_OutlineProgram,"u_OutlineWorld"), 0.01f);

        for(auto e: ctx.m_Scene->getSelected()){
            auto* tf = ctx.m_Scene->registry().try_get<TransformComponent>(e);
            auto* me = ctx.m_Scene->registry().try_get<MeshComponent>(e);
            if(!tf||!me||me->m_Indices.empty()) continue;
            auto it = m_Cache->find(e); if(it==m_Cache->end()) continue;
            glm::mat4 M = tf->GetTransform();
            glUniformMatrix4fv(glGetUniformLocation(m_OutlineProgram,"u_Model"),1,GL_FALSE,glm::value_ptr(M));
            glBindVertexArray(it->second.m_VAO);
            glDrawElements(GL_TRIANGLES,(GLsizei)me->m_Indices.size(),GL_UNSIGNED_INT,0);
        }

        // restore
        glCullFace(GL_BACK);
        glDisable(GL_CULL_FACE);
        glDisable(GL_STENCIL_TEST);
        glStencilMask(0xFF);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glBindVertexArray(0);
        glUseProgram(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

} // namespace Nova::Renderer::OpenGL