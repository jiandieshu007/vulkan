#pragma once

#include "RenderPass.h"

class GeometryPass : public RenderPass {
public:
	GeometryPass(vks::VulkanDevice& device) : RenderPass(device) {}

	virtual void prepare(VkQueue& queue) override;
	virtual void draw(VkCommandBuffer& cmdBuffer) override;
	virtual void update() override;
	struct UBO{
		glm::mat4 projection;
		glm::mat4 model;
		glm::mat4 view;
	} uboMvp;
};