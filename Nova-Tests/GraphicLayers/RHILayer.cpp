#include "RHILayer.h"

#include "Core/Application.h"
#include "Core/GraphicsAPI.h"
#include "Core/Log.h"
#include "Renderer/RHI/RHI_RenderGraphBuilder.h"

#include "EmptyLayer.h"

#include "imgui.h"

RHILayer::RHILayer() : Layer("RHILayer") {}

void RHILayer::OnAttach() {
    NV_LOG_INFO("RHILayer attached");

    auto& app = Nova::Core::Application::Get();
    auto& window = app.GetWindow();
    auto& imgui = app.GetImGuiLayer();

    const auto api = Nova::Core::GraphicsAPI::Vulkan;

    // Tear down previous ImGui renderer backend (e.g. SDLRenderer) before
    // destroying window-owned SDL resources and creating Vulkan.
    if (window.GetGraphicsAPI() == Nova::Core::GraphicsAPI::SDLRenderer)
        imgui.DestroyImGuiBackend(Nova::Core::GraphicsAPI::SDLRenderer);

    if (window.GetGraphicsAPI() != api) {
        if (!window.SetGraphicsAPI(api)) {
            NV_LOG_ERROR("RHILayer: failed to set GraphicsAPI::Vulkan");
            return;
        }
    }

    imgui.SetImGuiBackend(api);

    namespace RHI = Nova::Core::Renderer::RHI;

    RHI::RHI_SwapchainDesc swapDesc{};
    swapDesc.m_FramesInFlight = 3;
    swapDesc.m_CreateSurface = true;
    swapDesc.m_EnableSwapchain = true;
    swapDesc.m_PreferredPresentMode = RHI::RHI_PresentMode::Default;

    m_Renderer = RHI::IRenderer::Create(api, swapDesc);
    if (!m_Renderer) {
        NV_LOG_ERROR("RHILayer: failed to create RHI renderer");
        return;
    }

    int width = 0, height = 0;
    window.GetWindowSize(width, height);
    if (width <= 0 || height <= 0) {
        width = 1280;
        height = 720;
    }

    // Frame-graph declaration stays RHI (API-agnostic); only the backend is Vulkan.
    RHI::RHI_RenderGraphBuilder rg;
    const auto backbuffer = rg.ImportTexture({
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        RHI::RHI_TextureFormat::RGBA8,
        RHI::RHI_TextureUsage::ColorAttachment,
    }, RHI::RHI_ResourceState::Present);

    // Minimal present pass: clears the swapchain (black) and leaves rendering
    // open so ImGui can draw — same pattern as AppRenderer's "UI" pass.
    rg.AddPass("UI",
        [&](RHI::RHI_PassBuilder& b) {
            b.Write(backbuffer);
            b.PresentOnly();
        },
        [](RHI::IPassContext& /*ctx*/) {});

    m_Renderer->SetRenderGraph(rg.Build(api));
    NV_LOG_INFO("RHILayer: RHI renderer + present graph ready");
}

void RHILayer::OnDetach() {
    NV_LOG_INFO("RHILayer detached");

    if (m_Renderer) {
        Nova::Core::Application::Get().GetImGuiLayer().DestroyImGuiBackend(Nova::Core::GraphicsAPI::Vulkan);
        m_Renderer->Destroy();
        m_Renderer.reset();
    }
}

void RHILayer::OnUpdate(float /*dt*/) {
    if (m_Renderer)
        m_Renderer->Update(0.0f);
}

void RHILayer::OnBegin() {
    if (m_Renderer)
        m_Renderer->BeginFrame();
}

void RHILayer::OnRender() {
    if (m_Renderer)
        m_Renderer->RenderFrame();
}

void RHILayer::OnEnd() {
    if (m_Renderer)
        m_Renderer->EndFrame();
}

void RHILayer::OnImGuiRender() {
    ImGui::Begin("RHILayer");
    ImGui::Text("Graphics API: RHI");
    ImGui::Text("RHI_Renderer: %p", static_cast<void*>(m_Renderer.get()));
    ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Swapchain clear + ImGui via RHI");
    ImGui::End();
}

void RHILayer::OnEvent(Nova::Core::Events::Event& e) {
    Nova::Core::Events::EventDispatcher dispatcher(e);
    dispatcher.Dispatch<Nova::Core::Events::KeyPressedEvent>([this](Nova::Core::Events::KeyPressedEvent& ev) {
        return OnKeyPressed(ev);
    });
}

bool RHILayer::OnKeyPressed(Nova::Core::Events::KeyPressedEvent& e) {
    if (e.IsRepeat())
        return false;

    if (e.GetKeyCode() == SDLK_SPACE) {
        Nova::Core::Application::Get().GetLayerStack().QueueLayerTransition<EmptyLayer>(this);
        NV_LOG_INFO("RHILayer: transition to EmptyLayer requested");
        return true;
    }

    return false;
}