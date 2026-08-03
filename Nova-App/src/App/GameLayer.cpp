#include "App/GameLayer.h"

#include "App/AppLayer.h"
#include "Core/Assert.h"

namespace Nova::App {

    void GameLayer::OnAttach() {
        if (g_AppLayer) {
            g_AppLayer->RegisterGameLayer(this);
            g_AppLayer->ShowGrid(false);
        }
    }

    void GameLayer::OnDetach() {
        if (g_AppLayer) {
            g_AppLayer->RegisterGameLayer(nullptr);
            g_AppLayer->ShowGrid(true);
        }
    }

    void GameLayer::OnUpdate(float) {}

    void GameLayer::OnBegin() {}

    void GameLayer::OnRender() {}

    void GameLayer::OnEnd() {}

    void GameLayer::OnImGuiRender() {}

    void GameLayer::OnEvent(Nova::Core::Events::Event&) {}

} // namespace Nova::App