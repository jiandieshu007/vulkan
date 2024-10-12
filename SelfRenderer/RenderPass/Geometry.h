#pragma once

#include "./RenderPass.h"


class GeometryPass : public RenderPass {
public:
	GeometryPass(VulkanExampleBase* example) : RenderPass(example) {}
	virtual ~GeometryPass()
	{
		uboBuffer.destroy();
		vkDestroySemaphore(Device->logicalDevice, geometey, nullptr);
		vkFreeCommandBuffers(Device->logicalDevice, exampleBase->cmdPool, 1, &commandBuffer);
	}
	virtual void prepare() override;
	virtual void draw() override;
	virtual void update() override;
	virtual void createRenderpass() override;
	
	struct UBO{
		glm::mat4 projection;
		glm::mat4 model;
		glm::mat4 view;
	} uboMvp;

	vks::Buffer uboBuffer;
	VkSemaphore geometey;
	VkCommandBuffer commandBuffer;
	void updateUboBuffer();
};