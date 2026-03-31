#include "App/GameLayer.h"

#include "App/AppLayer.h"
#include "Core/Assert.h"

namespace Nova::App {

    void GameLayer::OnAttach() {
        if (g_AppLayer)
            g_AppLayer->RegisterGameLayer(this);
    }

    void GameLayer::OnDetach() {
        if (g_AppLayer)
            g_AppLayer->RegisterGameLayer(nullptr);
    }

    void GameLayer::OnUpdate(float) {}

    void GameLayer::OnBegin() {}

    void GameLayer::OnRender() {
        if (g_AppLayer)
            g_AppLayer->RenderScene();
    }

    void GameLayer::OnEnd() {}

    void GameLayer::OnImGuiRender() {}

    void GameLayer::OnEvent(Nova::Core::Events::Event&) {}

} // namespace Nova::App