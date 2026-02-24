#ifndef VKLAYER_H
#define VKLAYER_H

#include "Events/Event.h"
#include "Events/InputEvents.h"
#include "Events/ApplicationEvents.h"

#include "Core/Application.h"
#include "Core/Layer.h"
#include "Core/Log.h"

#include "Scene/ECS/Components/CameraComponent.h"
#include "Scene/ECS/Components/TransformComponent.h"

#include "Asset/AssetManager.h"
#include "Asset/Asset.h"
#include "Asset/Assets/MeshAsset.h"

#include "Renderer/Graphics/Camera.h"
#include "Renderer/RHI/RHI_Renderer.h"

#include "Scene/Scene.h"

using namespace Nova::Core;
using namespace Nova::Core::Events;
using namespace Nova::Core::Scene;
using namespace Nova::Core::Renderer;
using namespace Nova::Core::Renderer::Graphics;
using namespace Nova::Core::Scene::ECS::Components;

using namespace Nova::Core::Asset;
using namespace Nova::Core::Asset::Assets;

namespace Nova::App {

	class VkLayer : public Layer {
	public:
		explicit VkLayer(): Layer("VkLayer") {}
		~VkLayer() override;

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(float dt) override;
		void OnBegin() override;
		void OnRender() override;
		void OnEnd() override;
		void OnImGuiRender() override;
		void OnEvent(Event& e) override;
	
	private:
		// ---- Orbit camera helpers ----
		void UpdateCameraFromOrbit();
		void UpdateCameraAspectFromWindow();

		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
		bool OnMouseButtonReleased(MouseButtonReleasedEvent& e);
		bool OnMouseMoved(MouseMovedEvent& e);
		bool OnMouseScrolled(MouseScrolledEvent& e);
		bool OnWindowResized(WindowResizeEvent& e);

	private:
		std::unique_ptr<Nova::Core::Renderer::RHI::IRenderer> m_Renderer;
		float m_DeltaTime = 0.0f;

		Nova::Core::Scene::Scene m_Scene{"Scene_test"};

		std::shared_ptr<Camera> m_Camera;

		struct OrbitState {
			glm::vec3 m_Target{0.0f, 0.0f, 0.0f};
			float m_Distance = 5.0f;

			float m_Yaw = 0.0f;
			float m_Pitch = 0.0f;

			float m_RotateSensitivity = 0.25f;
			float m_ZoomSensitivity = 0.5f;

			bool m_IsRotating = false;
			bool m_HasLastMousePos = false;
			glm::vec2 m_LastMousePos{0.0f, 0.0f};
		} m_Orbit;
	};

} // namespace Nova::App

#endif // VKLAYER_H