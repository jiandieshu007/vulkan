#pragma once

#include "RenderPass.h"
#include "Geometry.h"
#include <random>




constexpr int LightCount = 32;

class LightPass : public RenderPass {
public:

	LightPass(VulkanExampleBase* example, GeometryPass* gPass) : RenderPass(example), geometryPass(gPass) {}
	~LightPass()
	{
		lightBuffer.destroy();
	}
	virtual void prepare() override;
	virtual void update() override;
	virtual void draw() override;
	void prepareLight();
	void updateLight();
	std::vector<PointLight> pointLights;
	vks::Buffer lightBuffer;

	GeometryPass* geometryPass;
};	