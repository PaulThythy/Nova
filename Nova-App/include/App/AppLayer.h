#ifndef APP_LAYER_H
#define APP_LAYER_H

#include "imgui.h"

#include "Core/Application.h"
#include "Core/Layer.h"

namespace Nova::App {

    class AppLayer : public Nova::Core::Layer {
    public:
        AppLayer(Nova::Core::Application& app) : Nova::Core::Layer("AppLayer"), m_App(app) {}

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(float dt) override;
        void OnRender() override;
        void OnImGuiRender() override;
        void OnEvent(/*Nova::Core::Event& e*/) override;

    private:
        Nova::Core::Application& m_App;

        bool m_ShowDemo = true;
        bool m_ShowAnotherWindow = false;
        float m_Time = 0.0f;
    };

} // namespace Nova::App

#endif // APP_LAYER_H