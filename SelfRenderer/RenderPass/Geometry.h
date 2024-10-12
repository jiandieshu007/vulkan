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
	


	vks::Buffer uboBuffer;
	VkSemaphore geometey;
	VkCommandBuffer commandBuffer;
	void updateUboBuffer();
};