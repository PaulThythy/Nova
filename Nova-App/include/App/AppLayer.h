#ifndef APP_LAYER_H
#define APP_LAYER_H

#include "imgui.h"

#include "Core/Layer.h"

namespace Nova::App {

    class AppLayer : public Nova::Core::Layer {
    public:
        AppLayer() : Nova::Core::Layer("AppLayer") {}

        void OnAttach() override { m_Time = 0.0f; }
        void OnUpdate(float dt) override { m_Time += dt; }
        void OnRender() override;
        void OnImGuiRender() override;
        void OnEvent(/*Nova::Core::Event& e*/) override;

    private:
        float m_Time = 0.0f;
        bool  m_ShowDemo = true;
        bool m_ShowAnotherWindow = false;
    };

} // namespace Nova::App

#endif // APP_LAYER_H