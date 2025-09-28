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

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(m_Program);
    glUniformMatrix4fv(glGetUniformLocation(m_Program, "u_View"), 1, GL_FALSE, &ctx.m_View[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(m_Program, "u_Projection"), 1, GL_FALSE, &ctx.m_Projection[0][0]);
    glUniform1f(glGetUniformLocation(m_Program, "u_Near"), ctx.m_Near);
    glUniform1f(glGetUniformLocation(m_Program, "u_Far"),  ctx.m_Far);

    auto& reg = ctx.m_Scene->Registry();
    auto view = reg.view<Components::TransformComponent, Components::MeshComponent>();
    
    for (auto ent : view) {
        auto it = m_MeshCache.find(ent);
        if (it == m_MeshCache.end()) continue;

        const auto& tc = view.get<Components::TransformComponent>(ent);
        const auto& mc = view.get<Components::MeshComponent>(ent);

        glm::mat4 model = tc.GetTransform();
        glUniformMatrix4fv(glGetUniformLocation(m_Program, "u_Model"), 1, GL_FALSE, &model[0][0]);

        glBindVertexArray(it->second.m_VAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)mc.m_Indices.size(), GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

} // namespace Nova::Renderer::OpenGL