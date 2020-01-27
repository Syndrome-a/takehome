#include "PostLayer.h"
#include "florp/app/Application.h"
#include "florp/game/SceneManager.h"

PostLayer::PostLayer() {
	florp::app::Application* app = florp::app::Application::Get();

	// The color buffer should be marked as shader readable, so that we generate a texture for it
	RenderBufferDesc mainColor = RenderBufferDesc();
	mainColor.ShaderReadable = true;
	mainColor.Attachment = RenderTargetAttachment::Color0;
	mainColor.Format = RenderTargetType::Color24;

	// Create our fullscreen quad mesh
	{
		float vert[] = {
			-1.0f, -1.0f,  0.0f, 0.0f,
			 1.0f, -1.0f,  1.0f, 0.0f,
			-1.0f,  1.0f,  0.0f, 1.0f,
			 1.0f,  1.0f,  1.0f, 1.0f
		};
		uint32_t indices[] = {
			0, 1, 2,
			1, 3, 2
		};
		florp::graphics::BufferLayout layout = {
			{ "inPosition", florp::graphics::ShaderDataType::Float2 },
			{ "inUV",       florp::graphics::ShaderDataType::Float2 }
		};

		myFullscreenQuad = std::make_shared<florp::graphics::Mesh>(vert, 4, layout, indices, 6);
	}
	
	// Our first pass will be to make our checkerboard effect
	if (true)
	{
		auto shader = std::make_shared<florp::graphics::Shader>();
		shader->LoadPart(florp::graphics::ShaderStageType::VertexShader, "shaders/post/post.vs.glsl");
		shader->LoadPart(florp::graphics::ShaderStageType::FragmentShader, "shaders/post/checker.fs.glsl");
		shader->Link();
		shader->SetUniform("xCheckerSize", 2);
		shader->SetUniform("xCheckerColor", glm::vec3(0.0f, 0.0f, 0.0f));

		// Each pass gets it's own copy of the frame buffer to avoid pipeline stalls
		auto output = std::make_shared<FrameBuffer>(app->GetWindow()->GetWidth(), app->GetWindow()->GetHeight());
		output->AddAttachment(mainColor);
		output->Validate();

		// Add the pass to the post processing stack
		myPasses.push_back({ shader, output });
	}

	// Our second pass will invert the colors
	if (false)
	{
		auto shader = std::make_shared<florp::graphics::Shader>();
		shader->LoadPart(florp::graphics::ShaderStageType::VertexShader, "shaders/post/post.vs.glsl");
		shader->LoadPart(florp::graphics::ShaderStageType::FragmentShader, "shaders/post/invert.fs.glsl");
		shader->Link();

		// We'll make a second framebuffer so that we can perform multiple post-processing effects
		auto output = std::make_shared<FrameBuffer>(app->GetWindow()->GetWidth(), app->GetWindow()->GetHeight());
		output->AddAttachment(mainColor);
		output->Validate();
		output->SetDebugName("InvertBuffer");

		// Add the pass to the post processing stack
		myPasses.push_back({ shader, output });
	}
}

void PostLayer::OnWindowResize(uint32_t width, uint32_t height) {
	//myMainFrameBuffer->Resize(width, height);
	for(auto& pass : myPasses)
		pass.Output->Resize(width, height);
}

void PostLayer::PostRender() {
	// We grab the application singleton to get the size of the screen
	florp::app::Application* app = florp::app::Application::Get();

	FrameBuffer::Sptr mainBuffer = CurrentRegistry().ctx<FrameBuffer::Sptr>();
	
	// Unbind the main framebuffer, so that we can read from it
	//mainBuffer->UnBind();
	glDisable(GL_DEPTH_TEST);

	// The last output will start as the output from the rendering
	FrameBuffer::Sptr lastPass = mainBuffer;

	// We'll iterate over all of our render passes
	for (const PostPass& pass : myPasses) {
		// We'll bind our post-processing output as the current render target and clear it
		pass.Output->Bind(RenderTargetBinding::Draw);
		glClear(GL_COLOR_BUFFER_BIT);
		// Set the viewport to be the entire size of the passes output
		glViewport(0, 0, pass.Output->GetWidth(), pass.Output->GetHeight());

		// Use the post processing shader to draw the fullscreen quad
		pass.Shader->Use();
		lastPass->GetAttachment(RenderTargetAttachment::Color0)->Bind(0);
		pass.Shader->SetUniform("xImage", 0);
		pass.Shader->SetUniform("xScreenRes", glm::ivec2(app->GetWindow()->GetWidth(), app->GetWindow()->GetHeight()));
		myFullscreenQuad->Draw();

		// Unbind the output pass so that we can read from it
		pass.Output->UnBind();

		// Update the last pass output to be this passes output
		lastPass = pass.Output;
	}
		
	// Bind the last buffer we wrote to as our source for read operations
	lastPass->Bind(RenderTargetBinding::Read);
	// Copies the image from lastPass into the default back buffer
	FrameBuffer::Blit({ 0, 0, lastPass->GetWidth(), lastPass->GetHeight()},
		{0, 0, app->GetWindow()->GetWidth(), app->GetWindow()->GetHeight()}, BufferFlags::All, florp::graphics::MagFilter::Nearest);

	// Unbind the last buffer from read operations, so we can write to it again later
	lastPass->UnBind();
}
