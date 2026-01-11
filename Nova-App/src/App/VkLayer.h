#ifndef VKLAYER_H
#define VKLAYER_H

#include "Events/Event.h"
#include "Events/InputEvents.h"
#include "Events/ApplicationEvents.h"

#include "Core/Layer.h"

using namespace Nova::Core::Events;
using namespace Nova::Core;

namespace Nova::App {

    class VkLayer : public Layer {
    public:
        explicit VkLayer(): Layer("VkLayer") {}
        ~VkLayer() override;

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(float dt) override;
        void OnRender() override;
        void OnImGuiRender() override;
        void OnEvent(Event& e) override;

    };

} // namespace Nova::App

#endif // VKLAYER_H