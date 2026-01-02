#ifndef EDITORLAYER_H
#define EDITORLAYER_H

#include <iostream>
#include <vector>
#include <glad/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Events/Event.h"
#include "Events/InputEvents.h"

#include "Core/Layer.h"
#include "Core/Application.h"

#include "App/AppLayer.h"

using namespace Nova::Core::Events;
using namespace Nova::Core;

namespace Nova::App {

    class EditorLayer : public Layer {
    public:
        explicit EditorLayer(): Layer("EditorLayer") {}
        ~EditorLayer() override;

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(float dt) override;
        void OnRender() override;
        void OnImGuiRender() override;
        void OnEvent(Event& e) override;

        struct Grid {
            GLuint m_GridProgram;
            GLuint m_VAO;
            float m_SmallStep = 1.0f;
            float m_MajorStep = 10.0f;
            float m_FadeDistance = 200.0f;

            glm::vec3 m_SmallColor{ 0.25f };
            glm::vec3 m_MajorColor{ 0.35f };
            glm::vec3 m_XAxisColor{ 1.0f, 0.0f, 0.0f };
            glm::vec3 m_ZAxisColor{ 0.0f, 1.0f, 0.0f };

            glm::mat4 m_ViewProj{ 1.0f };
            glm::mat4 m_InvViewProj{ 1.0f };
            glm::vec3 m_CameraPos{ 0.0f };

            Grid() {
                const char* vertexShaderSource = R"(#version 330 core
                out vec2 v_UV;

                // full-screen triangle sans VBO (utilise gl_VertexID)
                void main() {
                    vec2 p;
                    if (gl_VertexID == 0) p = vec2(-1.0, -1.0);
                    else if (gl_VertexID == 1) p = vec2( 3.0, -1.0);
                    else p = vec2(-1.0,  3.0);

                    v_UV = p * 0.5 + 0.5;
                    gl_Position = vec4(p, 0.0, 1.0);
                }
                )";

                const char* fragmentShaderSource = R"(#version 330 core
                in vec2 v_UV;
                out vec4 FragColor;

                uniform mat4 u_ViewProjection;
                uniform mat4 u_InvViewProjection;
                uniform vec3 u_CameraPos;

                uniform float u_SmallStep;
                uniform float u_MajorStep;
                uniform float u_FadeDistance;

                uniform vec3 u_SmallColor;
                uniform vec3 u_MajorColor;
                uniform vec3 u_XAxisColor;
                uniform vec3 u_ZAxisColor;

                float gridLine(vec2 coord, float step) {
                    // Anti-aliasing via dérivées écran
                    vec2 r = coord / step;
                    vec2 g = abs(fract(r - 0.5) - 0.5) / fwidth(r);
                    float line = 1.0 - min(min(g.x, g.y), 1.0);
                    return line;
                }

                void main() {
                    // reconstruct ray in world from NDC
                    vec2 ndc = v_UV * 2.0 - 1.0;

                    vec4 nearH = u_InvViewProjection * vec4(ndc, -1.0, 1.0);
                    vec4 farH  = u_InvViewProjection * vec4(ndc,  1.0, 1.0);
                    vec3 nearP = nearH.xyz / nearH.w;
                    vec3 farP  = farH.xyz  / farH.w;

                    vec3 dir = normalize(farP - nearP);

                    // intersect plane y = 0
                    float denom = dir.y;
                    if (abs(denom) < 1e-6) discard;

                    float t = -nearP.y / denom;
                    if (t < 0.0) discard;

                    vec3 pos = nearP + dir * t;

                    // compute depth so the grid is occluded by geometry
                    vec4 clip = u_ViewProjection * vec4(pos, 1.0);
                    float ndcDepth = clip.z / clip.w;
                    float depth = ndcDepth * 0.5 + 0.5;

                    // tiny bias toward camera so grid appears over a ground plane at y=0
                    gl_FragDepth = clamp(depth - 1e-5, 0.0, 1.0);

                    float dist = length(pos - u_CameraPos);
                    float fade = 1.0 - clamp(dist / u_FadeDistance, 0.0, 1.0);

                    vec2 xz = pos.xz;

                    float small = gridLine(xz, u_SmallStep);
                    float major = gridLine(xz, u_MajorStep);

                    // axes (X axis: z=0, Z axis: x=0)
                    float axX = 1.0 - smoothstep(0.0, fwidth(xz.y) * 2.0, abs(xz.y)); // z==0
                    float axZ = 1.0 - smoothstep(0.0, fwidth(xz.x) * 2.0, abs(xz.x)); // x==0

                    vec3 col = vec3(0.0);
                    float a = 0.0;

                    col += u_SmallColor * (small * 0.35);
                    a = max(a, small * 0.35);

                    col = mix(col, u_MajorColor, major);
                    a = max(a, major);

                    col = mix(col, u_XAxisColor, axX);
                    a = max(a, axX);

                    col = mix(col, u_ZAxisColor, axZ);
                    a = max(a, axZ);

                    a *= fade;

                    if (a < 0.01) discard;

                    FragColor = vec4(col, a);
                }
                )";

                // vertex shader
                GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
                glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
                glCompileShader(vertexShader);

                GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
                glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
                glCompileShader(fragmentShader);

                // link shaders
                m_GridProgram = glCreateProgram();
                glAttachShader(m_GridProgram, vertexShader);
                glAttachShader(m_GridProgram, fragmentShader);
                glLinkProgram(m_GridProgram);

                glGenVertexArrays(1, &m_VAO);
            }

            void draw() {
                glUseProgram(m_GridProgram);
                
                glUniformMatrix4fv(glGetUniformLocation(m_GridProgram, "u_ViewProjection"), 1, GL_FALSE, &m_ViewProj[0][0]);
                glUniformMatrix4fv(glGetUniformLocation(m_GridProgram, "u_InvViewProjection"), 1, GL_FALSE, &m_InvViewProj[0][0]);
                glUniform3fv(glGetUniformLocation(m_GridProgram, "u_CameraPos"), 1, &m_CameraPos[0]);

                glUniform1f(glGetUniformLocation(m_GridProgram, "u_SmallStep"), m_SmallStep);
                glUniform1f(glGetUniformLocation(m_GridProgram, "u_MajorStep"), m_MajorStep);
                glUniform1f(glGetUniformLocation(m_GridProgram, "u_FadeDistance"), m_FadeDistance);

                glUniform3fv(glGetUniformLocation(m_GridProgram, "u_SmallColor"), 1, &m_SmallColor[0]);
                glUniform3fv(glGetUniformLocation(m_GridProgram, "u_MajorColor"), 1, &m_MajorColor[0]);
                glUniform3fv(glGetUniformLocation(m_GridProgram, "u_XAxisColor"), 1, &m_XAxisColor[0]);
                glUniform3fv(glGetUniformLocation(m_GridProgram, "u_ZAxisColor"), 1, &m_ZAxisColor[0]);

                glBindVertexArray(m_VAO);
                glDrawArrays(GL_TRIANGLES, 0, 3);
                glBindVertexArray(0);
            }

            ~Grid() {
                if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
                if (m_GridProgram) glDeleteProgram(m_GridProgram);
            }
        };

    private:
        bool OnKeyReleased(KeyReleasedEvent& e);

        std::unique_ptr<Grid> m_Grid;
    };

} // namespace Nova::App

#endif // EDITORLAYER_H