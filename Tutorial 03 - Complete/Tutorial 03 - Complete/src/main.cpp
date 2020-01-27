#include "florp/app/Application.h"
#include "florp/game/BehaviourLayer.h"
#include "SceneBuildLayer.h"
#include "florp/game/ImGuiLayer.h"
#include <RenderLayer.h>
#include "PostLayer.h"

int main()
{
	{
		// Create our application
		florp::app::Application* app = new florp::app::Application();

		// Set up our layers
		app->AddLayer<florp::game::BehaviourLayer>();
		app->AddLayer<florp::game::ImGuiLayer>();
		app->AddLayer<SceneBuilder>();
		app->AddLayer<PostLayer>();
		app->AddLayer<RenderLayer>();

		app->Run();

		delete app;
	}

	std::cin.get();
	
	return 0;
} 
