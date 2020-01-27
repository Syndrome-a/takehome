#pragma once
#include "florp/app/ApplicationLayer.h"
#include "FrameBuffer.h"
#include "florp/graphics/Mesh.h"
#include "florp/graphics/Shader.h"

class PostLayer : public florp::app::ApplicationLayer
{
public:
	PostLayer();
	virtual void OnWindowResize(uint32_t width, uint32_t height) override;
	virtual void PostRender() override;

protected:
	FrameBuffer::Sptr myMainFrameBuffer;
	florp::graphics::Mesh::Sptr myFullscreenQuad;

	struct PostPass {
		florp::graphics::Shader::Sptr Shader;
		FrameBuffer::Sptr             Output;
	};

	std::vector<PostPass> myPasses;
};
