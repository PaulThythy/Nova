#include "Renderer/OpenGL/GL_RenderPass.hpp"
#include "Renderer/OpenGL/GL_Shader.hpp"

#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Nova::Renderer::OpenGL {

    void GL_DebugAABBPass::Init() {
        // Compile shaders
        std::string vertexPath = std::string(SHADER_DIR) + "/debugAABB.vert";
        std::string fragmentPath = std::string(SHADER_DIR) + "/debugAABB.frag";
        m_Program = LoadRenderShader(vertexPath, fragmentPath);

        if (m_Program == 0) {
            std::cerr << "Failed to load/compile shaders!" << std::endl;
        }

        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);
        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glBindVertexArray(0);
    }

    void GL_DebugAABBPass::Destroy() {
        if (m_VBO) { glDeleteBuffers(1, &m_VBO); m_VBO = 0; }
        if (m_VAO) { glDeleteVertexArrays(1, &m_VAO); m_VAO = 0; }
        if (m_Program) { glDeleteProgram(m_Program); m_Program = 0; }
    }

    void GL_DebugAABBPass::BuildBoxLineList(const glm::vec3& bmin, const glm::vec3& bmax,
                                            std::vector<glm::vec3>& out) {
        // 8 coins
        glm::vec3 p000{bmin.x,bmin.y,bmin.z};
        glm::vec3 p100{bmax.x,bmin.y,bmin.z};
        glm::vec3 p010{bmin.x,bmax.y,bmin.z};
        glm::vec3 p110{bmax.x,bmax.y,bmin.z};
        glm::vec3 p001{bmin.x,bmin.y,bmax.z};
        glm::vec3 p101{bmax.x,bmin.y,bmax.z};
        glm::vec3 p011{bmin.x,bmax.y,bmax.z};
        glm::vec3 p111{bmax.x,bmax.y,bmax.z};

        // 12 arêtes (24 sommets)
        AddSegment(p000, p100, out);
        AddSegment(p100, p110, out);
        AddSegment(p110, p010, out);
        AddSegment(p010, p000, out);

        AddSegment(p001, p101, out);
        AddSegment(p101, p111, out);
        AddSegment(p111, p011, out);
        AddSegment(p011, p001, out);

        AddSegment(p000, p001, out);
        AddSegment(p100, p101, out);
        AddSegment(p110, p111, out);
        AddSegment(p010, p011, out);
    }

    void GL_DebugAABBPass::Execute(const GL_RenderPassCtx& ctx) {
        if (!m_Program || !ctx.m_Scene) return;

        std::vector<glm::vec3> linesWS;                 //vertices in world space
        linesWS.reserve(ctx.m_NumberOfLights * 24);

        for (const auto& L : ctx.m_Lights) {
            // Directional : no AABB (infinite)
            if ((L.m_Type == 0 && !m_DrawDirectional) ||
                (L.m_Type == 1 && !m_DrawPointLights) ||
                (L.m_Type == 2 && !m_DrawSpotLights)) {
                continue;
            }

            float r = glm::max(0.0001f, L.m_Range);
            glm::vec3 bminWS = L.m_Position - glm::vec3(r);
            glm::vec3 bmaxWS = L.m_Position + glm::vec3(r);

            BuildBoxLineList(bminWS, bmaxWS, linesWS);
        }

        if (linesWS.empty()) return;

        // Upload & draw
        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, linesWS.size() * sizeof(glm::vec3), linesWS.data(), GL_DYNAMIC_DRAW);

        glUseProgram(m_Program);
        glm::mat4 VP = ctx.m_Projection * ctx.m_View;
        glUniformMatrix4fv(glGetUniformLocation(m_Program, "u_ViewProjection"), 1, GL_FALSE, glm::value_ptr(VP));
        glUniform3fv(glGetUniformLocation(m_Program, "u_Color"), 1, glm::value_ptr(m_Color));

        glEnable(GL_DEPTH_TEST);
        glLineWidth(m_LineWidth);
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(linesWS.size()));

        glBindVertexArray(0);
        glUseProgram(0);
    }

} // namespace Nova::Renderer::OpenGL