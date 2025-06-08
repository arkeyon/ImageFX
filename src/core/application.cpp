#include "safpch.h"
#include <memory>
#include "application.h"

#include "globals.h"

#include <imgui.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/backends/imgui_impl_glfw.h>

#include "input/application_event.h"

namespace saf {

	void DebugLayer::Update()
	{

	}

	void DebugLayer::OnEvent(saf::Event& e)
	{

	}

	void DebugLayer::Render(std::shared_ptr<Renderer2D> renderer)
	{

	}

	int printFPS() {
		static auto oldTime = std::chrono::high_resolution_clock::now();
		static int fps;
		fps++;


		static int second_fps = 0;

		if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - oldTime) >= std::chrono::seconds{ 1 }) {
			oldTime = std::chrono::high_resolution_clock::now();

			second_fps = fps;
			fps = 0;
		}

		return second_fps;
	}

	void DebugLayer::ImGuiRender()
	{
		ImGui::Begin("Debug");
		ImGui::Text("FPS: %d", printFPS());
		ImGui::End();
	}

	Application::Application(const nlohmann::json& args, std::string title)
		: m_RunArgs(args)
	{

	}

	Application::~Application()
	{

	}

	GraphicsApplication::GraphicsApplication(const nlohmann::json& args, std::string title)
		: Application(args, title)
	{
		m_Window = std::make_shared<Window>(1024, 768, title);
		m_Input = std::make_shared<Input>();
		global::g_Input = m_Input;
		m_Renderer2D = std::make_shared<Renderer2D>(m_Window->GetWidth(), m_Window->GetHeight());

		m_Projection2D = glm::mat4(1.f);
		float left = 0.f, right = static_cast<float>(m_Window->GetWidth());
		float top = 0.f, bottom = static_cast<float>(m_Window->GetHeight());

		m_Projection2D[0] = glm::vec4(2.f / (right - left), 0.f, 0.f, 0.f);
		m_Projection2D[1] = glm::vec4(0.f, 2.f / (bottom - top), 0.f, 0.f);
		m_Projection2D[2] = glm::vec4(0.f, 0.f, 1.f, 0.f);
		m_Projection2D[3] = glm::vec4(-(right + left) / (right - left), -(bottom + top) / (bottom - top), 0.f, 1.f);
	}

	GraphicsApplication::~GraphicsApplication()
	{
		m_Renderer2D->Shutdown();
		m_Window->Shutdown();
	}

	void GraphicsApplication::Init()
	{
		IFX_INFO("Application Init");

		m_Window->Init();
		m_Renderer2D->Init();

		m_Window->SetEventCallback([this](Event& e)
			{
				EventDispatcher d(e);
				d.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e)
					{
						m_Projection2D = glm::mat4(1.f);
						float left = 0.f, right = static_cast<float>(e.GetWidth());
						float top = 0.f, bottom = static_cast<float>(e.GetHeight());

						m_Projection2D[0] = glm::vec4(2.f / (right - left), 0.f, 0.f, 0.f);
						m_Projection2D[1] = glm::vec4(0.f, 2.f / (bottom - top), 0.f, 0.f);
						m_Projection2D[2] = glm::vec4(0.f, 0.f, 1.f, 0.f);
						m_Projection2D[3] = glm::vec4(-(right + left) / (right - left), -(bottom + top) / (bottom - top), 0.f, 1.f);

						return false;
					});

				OnEvent(e);

				for (auto layer : m_Layers)
				{
					layer->OnEvent(e);
				}

				m_Input->OnEvent(e);
			});

		m_DebugLayer = std::make_shared<DebugLayer>();
		m_Layers.push_back(m_DebugLayer);

	}

	const int maxfps = 60;

	void GraphicsApplication::Run()
	{
		IFX_INFO("Application Run");

		while (!m_Window->ShouldClose() && m_Running)
		{
			static auto oldTime = std::chrono::high_resolution_clock::now();
			const int us = 1000000 / maxfps;

			if (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - oldTime) < std::chrono::microseconds{ us })
			{
				continue;
			}
			oldTime = std::chrono::high_resolution_clock::now();

			Update();
			m_Window->Update();
			m_Renderer2D->Resize(m_Window->GetWidth(), m_Window->GetHeight());

			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			for (auto layer : m_Layers)
			{
				if (layer->IsVisible())
				{
					layer->Update();
					layer->ImGuiRender();
					layer->Render(m_Renderer2D);
				}
			}

			m_Renderer2D->Flush(m_Projection2D);
		}

	}

	void Application::Run()
	{
		IFX_INFO("Application Run");

		while (m_Running)
		{
			Update();
		}

	}

}