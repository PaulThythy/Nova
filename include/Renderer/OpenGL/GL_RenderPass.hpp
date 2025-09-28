#ifndef GL_RENDER_PASS_HPP
#define GL_RENDER_PASS_HPP

#include "Scene/Scene.hpp"
#include "Renderer/OpenGL/GL_RenderPassCtx.hpp"

namespace Nova::Renderer::OpenGL {
	struct IGL_RenderPass {
		virtual ~IGL_RenderPass() = default;
		virtual void Init();
		virtual void Destroy();
		virtual void Execute(const GL_RenderPassCtx& ctx) = 0;
	};

	inline void IGL_RenderPass::Init() {}
	inline void IGL_RenderPass::Destroy() {}

	struct GL_DepthPrePass : public IGL_RenderPass {
		void Init() override;
		void Destroy() override;
		void Execute(const GL_RenderPassCtx& ctx) override;
		
		unsigned m_Program = 0;
    };

	struct GL_GBufferPass : public IGL_RenderPass {
		void Init() override;
		void Destroy() override;
		void Execute(const GL_RenderPassCtx& ctx) override;

		unsigned m_Program = 0;
	};

	struct GL_LightCullingPass : public IGL_RenderPass {
		void Init() override;
		void Destroy() override;
		void Execute(const GL_RenderPassCtx& ctx) override;

		//unsigned m_Program = 0;
	};
	
	struct GL_LightingPass : public IGL_RenderPass {
		void Init() override;
		void Destroy() override;
		void Execute(const GL_RenderPassCtx& ctx) override;

		unsigned m_Program = 0;
	};

	struct GL_PostProcessPass : public IGL_RenderPass {
		void Init() override;
		void Destroy() override;
		void Execute(const GL_RenderPassCtx& ctx) override;

		unsigned m_Program = 0;
	};

	struct GL_OutlinePass : public IGL_RenderPass {
		void Init() override;
		void Destroy() override;
		void Execute(const GL_RenderPassCtx& ctx) override;

		unsigned m_Program = 0;
	};

} // namespace Nova::Renderer::OpenGL

#endif // GL_RENDER_PASS_HPP