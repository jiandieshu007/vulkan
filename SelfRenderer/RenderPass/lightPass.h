#pragma once

#include "RenderPass.h"
#include "Geometry.h"
#include <random>


class LightPass : public RenderPass {
public:

	explicit LightPass(VulkanExampleBase* example, GeometryPass* gPass, ShadowPass* spass) : RenderPass(example), geometryPass(gPass), shadowPass(spass) {}
	~LightPass()
	{
		lightBuffer.destroy();
	}
	virtual void prepare() override;
	virtual void update() override;
	virtual void draw() override;
	virtual void createRenderpass() override;
	void prepareLight();
	void updateLight();
	std::vector<PointLight> pointLights;
	vks::Buffer lightBuffer;

	GeometryPass* geometryPass;
	ShadowPass* shadowPass;

};	