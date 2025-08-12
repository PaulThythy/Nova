#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#include "Renderer/OpenGL/RenderPass.hpp"
#include "Renderer/OpenGL/Passes/ShadowPass.hpp"
#include "Renderer/OpenGL/Shader.hpp"

#include "Components/TransformComponent.hpp"
#include "Components/MeshComponent.hpp"
#include "Components/MeshRendererComponent.hpp"

using namespace Nova::Components;

namespace Nova::Renderer::OpenGL {

    ShadowPass::ShadowPass(std::unordered_map<entt::entity, GLMeshBuffers>* c, unsigned* fbo, unsigned* tex, int* size) : m_Cache(c), m_FBO(fbo), m_Tex(tex), m_Size(size) {

        std::string vertexPath = std::string(SHADER_DIR) + "/shadowDepth.vert";
        std::string fragmentPath = std::string(SHADER_DIR) + "/shadowDepth.frag";
        m_Program = loadRenderShader(vertexPath, fragmentPath);

        if (m_Program == 0) {
            std::cerr << "Failed to load/compile shaders!" << std::endl;
        }
    }

    void ShadowPass::execute(const RenderContext& ctx){
        glViewport(0,0,*m_Size,*m_Size);
        glBindFramebuffer(GL_FRAMEBUFFER,*m_FBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);

        glUseProgram(m_Program);
        const GLint uLVP = glGetUniformLocation(m_Program, "u_LightViewProj");
        const GLint uM   = glGetUniformLocation(m_Program, "u_Model");
        glUniformMatrix4fv(uLVP,1,GL_FALSE,glm::value_ptr(ctx.m_LightVP));

        ctx.m_Scene->forEach<TransformComponent, MeshComponent, MeshRendererComponent>([
            &](entt::entity e, TransformComponent& tf, MeshComponent& mesh, MeshRendererComponent& mr){
                if(!mr.m_CastShadows) return;
                if(mesh.m_Indices.empty()) return;
                auto it = m_Cache->find(e); if(it==m_Cache->end()) return;
                glm::mat4 M = tf.GetTransform();
                glUniformMatrix4fv(uM,1,GL_FALSE,glm::value_ptr(M));
                glBindVertexArray(it->second.m_VAO);
                glDrawElements(GL_TRIANGLES, (GLsizei)mesh.m_Indices.size(), GL_UNSIGNED_INT, 0);
        });

        glBindVertexArray(0);
        glUseProgram(0);

        glCullFace(GL_BACK);
        glDisable(GL_CULL_FACE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
} // namespace Nova::Renderer::OpenGL