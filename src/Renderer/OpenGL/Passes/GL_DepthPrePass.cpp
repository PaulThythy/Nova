#include "Renderer/OpenGL/GL_RenderPass.hpp"
#include "Renderer/OpenGL/GL_RenderPassCtx.hpp"
#include "Renderer/OpenGL/GL_Caches.hpp"
#include "Renderer/OpenGL/GL_Shader.hpp"

#include "Components/TransformComponent.hpp"
#include "Components/MeshComponent.hpp"

#include <iostream>

namespace Nova::Renderer::OpenGL {

    void GL_DepthPrePass::Init() {
        // Compile shaders
        std::string vertexPath = std::string(SHADER_DIR) + "/depthPrePass.vert";
        std::string fragmentPath = std::string(SHADER_DIR) + "/depthPrePass.frag";
        m_Program = LoadRenderShader(vertexPath, fragmentPath);

        if (m_Program == 0) {
            std::cerr << "Failed to load/compile shaders!" << std::endl;
        }
    }

    void GL_DepthPrePass::Destroy() {
        if (m_Program) {
            glDeleteProgram(m_Program);
            m_Program = 0;
        }
    }

    void GL_DepthPrePass::Execute(const GL_RenderPassCtx& ctx) {
        if (!m_Program || !ctx.m_FBO || !ctx.m_Scene) return;

        glBindFramebuffer(GL_FRAMEBUFFER, ctx.m_FBO);
        glViewport(0, 0, ctx.m_Width, ctx.m_Height);

        // DEPTH-ONLY
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);                   // write the closest fragment

        glClear(GL_DEPTH_BUFFER_BIT);

        glUseProgram(m_Program);
        glUniformMatrix4fv(glGetUniformLocation(m_Program, "u_View"), 1, GL_FALSE, &ctx.m_View[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(m_Program, "u_Projection"), 1, GL_FALSE, &ctx.m_Projection[0][0]);

        auto& reg = ctx.m_Scene->Registry();
        auto view = reg.view<Components::TransformComponent, Components::MeshComponent>();
        
        view.each([&](auto ent, const auto& tc, const auto& mc) {
            auto it = m_MeshCache.find(ent);
            if (it == m_MeshCache.end()) return;

            const auto& transformComp = view.get<Components::TransformComponent>(ent);
            const auto& meshComp = view.get<Components::MeshComponent>(ent);

            glm::mat4 model = transformComp.GetTransform();
            glUniformMatrix4fv(glGetUniformLocation(m_Program, "u_Model"), 1, GL_FALSE, &model[0][0]);

            glBindVertexArray(it->second.m_VAO);
            glDrawElements(GL_TRIANGLES, (GLsizei)meshComp.m_Indices.size(), GL_UNSIGNED_INT, 0);
        });

        glUseProgram(0);

        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }

} // namespace Nova::Renderer::OpenGL