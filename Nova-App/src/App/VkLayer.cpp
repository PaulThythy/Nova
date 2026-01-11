#include "App/VkLayer.h"

namespace Nova::App {

    VkLayer::~VkLayer() = default;

    void VkLayer::OnAttach() {}

    void VkLayer::OnDetach() {}

    void VkLayer::OnUpdate(float dt) {}

    void VkLayer::OnRender() {}

    void VkLayer::OnEvent(Event& event) {}

    void VkLayer::OnImGuiRender() {}

}// namespace Nova::App