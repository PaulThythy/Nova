#include "Renderer/OpenGL/GL_RenderPass.hpp"
#include "Renderer/OpenGL/GL_RenderPassCtx.hpp"
#include "Renderer/OpenGL/GL_Caches.hpp"
#include "Renderer/OpenGL/GL_Shader.hpp"

#include <iostream>
#include <filesystem>

namespace Nova::Renderer::OpenGL {

    void GL_GBufferPass::Init() {
        // Compile shaders
        std::filesystem::path currentPath = std::filesystem::current_path();
        std::filesystem::path shaderDir = currentPath / "shaders" / "glsl";

        std::string vertexPath   = (shaderDir / "gBufferPass.vert").string();
        std::string fragmentPath = (shaderDir / "gBufferPass.frag").string();
        m_Program = LoadRenderShader(vertexPath, fragmentPath);

        if (m_Program == 0) {
            std::cerr << "Failed to load/compile shaders!" << std::endl;
        }
    }

    void GL_GBufferPass::Destroy() {
        if (m_Program) {
            glDeleteProgram(m_Program);
            m_Program = 0;
        }
    }

    void GL_GBufferPass::Execute(const GL_RenderPassCtx& ctx) {
        if (!m_Program || !ctx.m_FBO || !ctx.m_Scene) return;

        glBindFramebuffer(GL_FRAMEBUFFER, ctx.m_FBO);
        glViewport(0, 0, ctx.m_Width, ctx.m_Height);

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);
        glDisable(GL_BLEND);

        // to prevent z-fighting with depth pre-pass
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-1.0f, -1.0f);

        // === CLEAR 4 ATTACHMENTS ===
        // Position (rgba16f) -> 0
        {
            const GLfloat clearPos[4] = {0.f, 0.f, 0.f, 0.f};
            glClearBufferfv(GL_COLOR, 0, clearPos);
        }
        // Normal (rgba16f) -> default normal (0,0,1), a=0
        {
            const GLfloat clearNrm[4] = {0.f, 0.f, 1.f, 0.f};
            glClearBufferfv(GL_COLOR, 1, clearNrm);
        }
        // Albedo+Roughness (rgba8) -> black + roughness=1 by default
        {
            const GLfloat clearAlbRou[4] = {0.f, 0.f, 0.f, 1.f};
            glClearBufferfv(GL_COLOR, 2, clearAlbRou);
        }
        // Metallic (r8) -> 0
        {
            const GLfloat clearMet[4] = {0.f, 0.f, 0.f, 0.f};
            glClearBufferfv(GL_COLOR, 3, clearMet);
        }
        // ================================

        glUseProgram(m_Program);
        glUniformMatrix4fv(glGetUniformLocation(m_Program, "u_View"), 1, GL_FALSE, &ctx.m_View[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(m_Program, "u_Projection"), 1, GL_FALSE, &ctx.m_Projection[0][0]);

        auto& reg = ctx.m_Scene->Registry();
        auto view = reg.view<Components::TransformComponent, Components::MeshComponent, Components::MeshRendererComponent>();
        
        view.each([&](auto ent, const auto& tc, const auto& mc, const auto& mrc) {
            auto it = m_MeshCache.find(ent);
            if (it == m_MeshCache.end()) return;

            const auto& transformComp = view.get<Components::TransformComponent>(ent);
            const auto& meshComp = view.get<Components::MeshComponent>(ent);
            const auto& meshRendererComp = view.get<Components::MeshRendererComponent>(ent);

            glm::mat4 model = transformComp.GetTransform();
            glm::mat3 normalMat = glm::mat3(glm::transpose(glm::inverse(model)));
            
            glUniformMatrix4fv(glGetUniformLocation(m_Program, "u_Model"), 1, GL_FALSE, &model[0][0]);
            glUniformMatrix3fv(glGetUniformLocation(m_Program, "u_NormalMatrix"), 1, GL_FALSE, &normalMat[0][0]);
            glUniform3fv(glGetUniformLocation(m_Program, "u_BaseColor"), 1, &mrc.m_BaseColor[0]);
            glUniform1f(glGetUniformLocation(m_Program, "u_Roughness"), mrc.m_Roughness);
            glUniform1f(glGetUniformLocation(m_Program, "u_Metallic"),  mrc.m_Metallic);

            glBindVertexArray(it->second.m_VAO);
            glDrawElements(GL_TRIANGLES, (GLsizei)mc.m_Indices.size(), GL_UNSIGNED_INT, 0);
        });

        glBindVertexArray(0);
        glUseProgram(0);

        glDisable(GL_POLYGON_OFFSET_FILL);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
    }

} // namespace Nova::Renderer::OpenGL