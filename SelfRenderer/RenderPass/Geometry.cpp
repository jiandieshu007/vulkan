#include "Geometry.h"

//, vkglTF::descriptorSetLayoutImage 
void GeometryPass::prepare(VkQueue& queue)
{
	loadScene(getAssetPath() + "models/sponza/sponza.gltf", queue);

	// Four attachments (3 color, 1 depth)
	vks::AttachmentCreateInfo attachmentInfo = {};
	attachmentInfo.width = 2048;
	attachmentInfo.height = 2048;
	attachmentInfo.layerCount = 1;
	attachmentInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	// attachment0 color
	attachmentInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	FrameBuffer.addAttachment(attachmentInfo);

	// Color attachments
// Attachment 0: (World space) Positions
	attachmentInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	FrameBuffer.addAttachment(attachmentInfo);

	// Attachment 1: (World space) Normals
	attachmentInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	FrameBuffer.addAttachment(attachmentInfo);

	// Attachment 2: Albedo (color)
	attachmentInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	FrameBuffer.addAttachment(attachmentInfo);

	// Depth attachment
	// Find a suitable depth format
	VkFormat attDepthFormat;
	VkBool32 validDepthFormat = vks::tools::getSupportedDepthFormat(Device.physicalDevice, &attDepthFormat);
	assert(validDepthFormat);

	attachmentInfo.format = attDepthFormat;
	attachmentInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	FrameBuffer.addAttachment(attachmentInfo);

	FrameBuffer.createSampler(VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	FrameBuffer.createRenderPass();

	// Pool
	std::vector<VkDescriptorPoolSize> poolSizes = {
		vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
		vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
	};
	VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 2);
	VK_CHECK_RESULT(vkCreateDescriptorPool(Device.logicalDevice, &descriptorPoolInfo, nullptr, &DescriptorPool));


}
