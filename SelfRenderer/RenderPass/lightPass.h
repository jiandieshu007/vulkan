#pragma once

#include "RenderPass.h"
#include "Geometry.h"
#include <random>
#include "shadowPass.h"


struct pointLightWithInvView
{
	
};

class LightPass : public RenderPass {
public:
	using depthsArray = vks::FramebufferAttachment;
	explicit LightPass(VulkanExampleBase* example, GeometryPass* gPass, ShadowPass* spass) : RenderPass(example), geometryPass(gPass), shadowPass(spass) {}
	~LightPass()
	{
		lightBuffer.destroy();
		vkDestroyImageView(Device->logicalDevice, DepthsCubeArray.view, nullptr);
		vkDestroyImage(Device->logicalDevice, DepthsCubeArray.image, nullptr);
	}
	virtual void prepare() override;
	virtual void update(VkQueue& queue) override;
	virtual void draw() override;
	virtual void createRenderpass() override;
	void prepareLight();
	void updateLight();
	void createPointLightDepthsimage(const pointLightsWithFramebuffer& buf, depthsArray* texture)
	{
		texture->format = buf.framebuffer[0]->attachments[0].format;

		VkImageCreateInfo imageCreateInfo = vks::initializers::imageCreateInfo();
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = texture->format;
		imageCreateInfo.extent.width = shadowPass->depthsWidth;
		imageCreateInfo.extent.height = shadowPass->depthsHeight;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 6 * LightCount;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		VK_CHECK_RESULT(vkCreateImage(Device->logicalDevice, &imageCreateInfo, nullptr, &texture->image))

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(Device->logicalDevice, texture->image, &memRequirements);

		VkMemoryAllocateInfo allocInfo = vks::initializers::memoryAllocateInfo();
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = Device->getMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VK_CHECK_RESULT(vkAllocateMemory(Device->logicalDevice, &allocInfo, nullptr, &texture->memory));
		VK_CHECK_RESULT(vkBindImageMemory(Device->logicalDevice, texture->image, texture->memory, 0));


		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = texture->image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
		viewInfo.format = texture->format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT ;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 6*LightCount;


		VK_CHECK_RESULT(vkCreateImageView(Device->logicalDevice, &viewInfo, nullptr, &texture->view))
	}

	void updatePointLightDepthsimage(VkQueue queue, const std::vector<pointLightsWithFramebuffer>& buf, depthsArray* texture)
	{
		VkCommandBuffer copyBuf = Device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		for (uint32_t i = 0; i < LightCount; ++i)
		{
			for (int j = 0; j < 6; ++j) {
				VkImageCopy copyRegion{};
				copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				copyRegion.srcSubresource.mipLevel = 1;
				copyRegion.srcSubresource.baseArrayLayer = 0;
				copyRegion.srcSubresource.layerCount = 1;
				copyRegion.srcOffset = { 0,0,0 };

				copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				copyRegion.dstSubresource.mipLevel = 1;
				copyRegion.dstSubresource.baseArrayLayer = 0;
				copyRegion.dstSubresource.layerCount = i*6 + j ;
				copyRegion.dstOffset = { 0,0,0 };

				copyRegion.extent.width = shadowPass->depthsWidth;
				copyRegion.extent.height = shadowPass->depthsHeight;
				copyRegion.extent.height = 1;

				vkCmdCopyImage(copyBuf, buf[i].framebuffer[j]->attachments[0].image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
			}
		}
		vkEndCommandBuffer(copyBuf);

		VkSubmitInfo submitInfo = vks::initializers::submitInfo();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &copyBuf;
		vkQueueSubmit(queue, 1, &submitInfo, nullptr);
	}

	std::vector<PointLight> pointLights;
	vks::Buffer lightBuffer;
	depthsArray DepthsCubeArray;

	GeometryPass* geometryPass;
	ShadowPass* shadowPass;

};	