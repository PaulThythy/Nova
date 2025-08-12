#ifndef OPENGL_RENDERER_HPP
#define OPENGL_RENDERER_HPP

#include <GL/glew.h>

#include "Renderer/Renderer.hpp"
#include "Scene/Scene.hpp"
#include "Components/MeshComponent.hpp"
#include "Components/CameraComponent.hpp"
#include "Components/TransformComponent.hpp"
#include "Components/LightComponent.hpp"
#include "Components/MeshRendererComponent.hpp"

namespace Nova::Renderer::OpenGL {

    class OpenGLRenderer : public IRenderer {
    public:
        void init(Nova::Scene& scene) override;
        void render() override;
        void destroy() override;

        void updateViewportSize(int width, int height) override;

        void* getImGuiTextureID() const override { return reinterpret_cast<void*>(m_ColorTexture); }

        int m_ViewportWidth = 1280;
        int m_ViewportHeight = 720;

    private:
        GLuint m_ShaderProgram = 0;
        GLuint m_OutlineProgram = 0;
        GLuint m_FBO = 0;
        GLuint m_ColorTexture = 0;
        GLuint m_DepthBuffer = 0;
        Nova::Scene* m_Scene = nullptr;

        void initFBO(int width, int height);

        GLuint m_ShadowFBO = 0;
        GLuint m_ShadowDepthTex = 0;
        int m_ShadowSize = 2048;
        GLuint m_ShadowProgram = 0;
        glm::mat4 m_LastLightVP = glm::mat4(1.0f);

        void initShadows();
        void renderShadowPass(const glm::mat4& lightVP);

        void onMeshDestroyed(entt::registry& reg, entt::entity ent);
        void onMeshConstructed(entt::registry& reg, entt::entity ent);

        struct GLMeshBuffers {
            GLuint m_VAO, m_VBO, m_IBO;
        };
        std::unordered_map<entt::entity, GLMeshBuffers> m_MeshCache;
        GLMeshBuffers createGLMeshBuffers(const Nova::Components::MeshComponent& mesh);
    };
} // namespace Nova::Renderer::OpenGL

#endif // OPENGL_RENDERER_HPP