#include "SceneBuildLayer.h"
#include "florp/game/SceneManager.h"
#include "florp/game/RenderableComponent.h"
#include <florp\graphics\MeshData.h>
#include <florp\graphics\MeshBuilder.h>
#include <florp\graphics\ObjLoader.h>

#include <glm/gtc/matrix_transform.hpp>
#include <florp\game\Transform.h>
#include "SampleBehaviour.h"
#include "CameraComponent.h"
#include "florp/app/Application.h"

void SceneBuilder::Initialize()
{
	florp::app::Application* app = florp::app::Application::Get();
	
	using namespace florp::game;
	using namespace florp::graphics;
	
	auto* scene = SceneManager::RegisterScene("main");
	SceneManager::SetCurrentScene("main");

	MeshData data = ObjLoader::LoadObj("monkey.obj", glm::vec4(1.0f));

	Shader::Sptr shader = std::make_shared<Shader>();
	shader->LoadPart(ShaderStageType::VertexShader, "shaders/lighting.vs.glsl");
	shader->LoadPart(ShaderStageType::FragmentShader, "shaders/blinn-phong-multi.fs.glsl"); 
	shader->Link();

	Material::Sptr mat = std::make_shared<Material>(shader);
	mat->Set("a_AmbientColor", { 1.0f, 1.0f, 1.0f });
	mat->Set("a_AmbientPower", 0.1f);
	mat->Set("a_MatShininess", 256.0f);
	mat->Set("a_Lights[0].Pos", { 2, 0, 0 });
	mat->Set("a_Lights[0].Color", { 1.0f, 0.0f, 1.0f });  
	mat->Set("a_Lights[0].Attenuation", 1.0f / 100.0f);
	mat->Set("a_Lights[1].Pos", { -2, 0, 0 });
	mat->Set("a_Lights[1].Color", { 0.0f, 1.0f, 0.0f });
	mat->Set("a_Lights[1].Attenuation", 1.0f / 100.0f);
	mat->Set("a_EnabledLights", 2);
	
	// Our spinning monkey head
	{
		entt::entity test = scene->CreateEntity();
		//scene->AddBehaviour<SampleBehaviour>(test, "Hello world!");
		RenderableComponent& renderable = scene->Registry().assign<RenderableComponent>(test);
		renderable.Mesh = MeshBuilder::Bake(data);
		renderable.Material = mat;
		Transform& t = scene->Registry().get<Transform>(test);
		scene->AddBehaviour<SampleBehaviour>(test, glm::vec3(0,45.0f,90.0f));
		t.SetPosition(glm::vec3(0, 0.0f, 0.0f));
	}

	// Creates our main camera
	{
		// The color buffer should be marked as shader readable, so that we generate a texture for it
		RenderBufferDesc mainColor = RenderBufferDesc();
		mainColor.ShaderReadable = true;
		mainColor.Attachment = RenderTargetAttachment::Color0;
		mainColor.Format = RenderTargetType::Color24;

		// The depth attachment does not need to be a texture (and would cause issues since the format is DepthStencil)
		RenderBufferDesc depth = RenderBufferDesc();
		depth.Attachment = RenderTargetAttachment::DepthStencil;
		depth.Format = RenderTargetType::DepthStencil;

		// Our main frame buffer needs a color output, and a depth output
		FrameBuffer::Sptr buffer = std::make_shared<FrameBuffer>(app->GetWindow()->GetWidth(), app->GetWindow()->GetHeight());
		buffer->AddAttachment(mainColor);
		buffer->AddAttachment(depth);
		buffer->Validate();
		buffer->SetDebugName("MainBuffer");
		
		entt::entity camera = scene->CreateEntity();
		CameraComponent& cam = scene->Registry().assign<CameraComponent>(camera);
		cam.Buffer = buffer;
		cam.IsMainCamera = true;
		cam.Projection = glm::perspective(glm::radians(60.0f), 1.0f, 0.01f, 1000.0f);
		Transform& t = scene->Registry().get<Transform>(camera);
		t.SetPosition(glm::vec3(0.0f, 0.0f, 5.0f));
		t.LookAt(glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	FrameBuffer::Sptr secondaryCamBuffer;
	// Creates our secondary camera
	{
		// The color buffer should be marked as shader readable, so that we generate a texture for it
		RenderBufferDesc mainColor = RenderBufferDesc();
		mainColor.ShaderReadable = true;
		mainColor.Attachment = RenderTargetAttachment::Color0;
		mainColor.Format = RenderTargetType::Color24;

		// The depth attachment does not need to be a texture (and would cause issues since the format is DepthStencil)
		RenderBufferDesc depth = RenderBufferDesc();
		depth.Attachment = RenderTargetAttachment::DepthStencil;
		depth.Format = RenderTargetType::DepthStencil;

		// Our main frame buffer needs a color output, and a depth output
		secondaryCamBuffer = std::make_shared<FrameBuffer>(app->GetWindow()->GetWidth(), app->GetWindow()->GetHeight(), 4);
		secondaryCamBuffer->AddAttachment(mainColor);
		secondaryCamBuffer->AddAttachment(depth);
		secondaryCamBuffer->Validate();
		secondaryCamBuffer->SetDebugName("SecondaryBuffer");

		// We'll create a second camera, and set it up to be our 'TV' camera
		entt::entity camera = scene->CreateEntity();
		CameraComponent& cam = scene->Registry().assign<CameraComponent>(camera);
		cam.Buffer = secondaryCamBuffer;
		cam.Projection = glm::perspective(glm::radians(60.0f), 1.0f, 0.01f, 1000.0f);
		cam.ClearCol = glm::vec4(0.5f, 0.0f, 0.5f, 1.0f);
		Transform& t = scene->Registry().get<Transform>(camera);
		t.SetPosition(glm::vec3(0.0f, 5.0f, 5.0f));
		t.LookAt(glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	Shader::Sptr textureShader = std::make_shared<Shader>();
	textureShader->LoadPart(ShaderStageType::VertexShader, "shaders/lighting.vs.glsl");
	textureShader->LoadPart(ShaderStageType::FragmentShader, "shaders/textured.fs.glsl");
	textureShader->Link();

	Material::Sptr tv = std::make_shared<Material>(textureShader);
	tv->Set("a_Sampler", secondaryCamBuffer);
	
	// Creates a cube that draws using the second camera's image
	{
		MeshData data = MeshBuilder::Begin();
		MeshBuilder::AddAlignedCube(data, glm::vec3(0.0f), glm::vec3(0.5f));
		Mesh::Sptr mesh = MeshBuilder::Bake(data);

		entt::entity entity = scene->CreateEntity();
		RenderableComponent& renderable = scene->Registry().assign<RenderableComponent>(entity);
		renderable.Mesh = MeshBuilder::Bake(data);
		renderable.Material = tv;
		Transform& t = scene->Registry().get<Transform>(entity);
		t.SetPosition(glm::vec3(0, 2.0f, 0.0f));
	}
}
