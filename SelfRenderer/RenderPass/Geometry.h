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
	virtual void update(VkQueue& queue) override;
	virtual void createRenderpass() override;
	UBO uboMvp;
	vks::Buffer uboBuffer;
	VkSemaphore geometey;
	VkCommandBuffer commandBuffer;
	void updateUboBuffer();
};