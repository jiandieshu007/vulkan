#include "Geometry.h"
#include "vulkanexamplebase.h"

//, vkglTF::descriptorSetLayoutImage 
void GeometryPass::prepare()
{
	scene = new vkglTF::Model();
	loadScene(getAssetPath() + "models/sponza/sponza.gltf", exampleBase->queue);

	FrameBuffer = new vks::Framebuffer(Device);

	// Four attachments (3 color, 1 depth)
	vks::AttachmentCreateInfo attachmentInfo = {};
	attachmentInfo.width = exampleBase->width;
	attachmentInfo.height = exampleBase->height;
	attachmentInfo.layerCount = 1;
	attachmentInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	FrameBuffer->width = exampleBase->width;
	FrameBuffer->height = exampleBase->height;

	// Color attachments
	// Attachment 0: (World space) Positions
	attachmentInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	FrameBuffer->addAttachment(attachmentInfo);

	// Attachment 1: (World space) Normals
	attachmentInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	FrameBuffer->addAttachment(attachmentInfo);

	// Attachment 2: Albedo (color)
	attachmentInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	FrameBuffer->addAttachment(attachmentInfo);

	// Depth attachment
	// Find a suitable depth format
	VkFormat attDepthFormat;
	VkBool32 validDepthFormat = vks::tools::getSupportedDepthFormat(Device->physicalDevice, &attDepthFormat);
	assert(validDepthFormat);

	attachmentInfo.format = attDepthFormat;
	attachmentInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	FrameBuffer->addAttachment(attachmentInfo);

	FrameBuffer->createSampler(VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	FrameBuffer->createRenderPass();



	Device->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&uboBuffer, sizeof(UBO));
	updateUboBuffer();

	// Pool
	std::vector<VkDescriptorPoolSize> poolSizes = {
		vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
		vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
	};
	VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 1);
	VK_CHECK_RESULT(vkCreateDescriptorPool(Device->logicalDevice, &descriptorPoolInfo, nullptr, &DescriptorPool));
	
	std::vector<VkDescriptorSetLayoutBinding> Bindings = {
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,VK_SHADER_STAGE_VERTEX_BIT,0)
	};
	VkDescriptorSetLayoutCreateInfo SetLayoutCreateInfo = vks::initializers::descriptorSetLayoutCreateInfo(Bindings);

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(Device->logicalDevice, &SetLayoutCreateInfo, nullptr, &DescriptorSetLayout));
	VkDescriptorSetAllocateInfo SetAllocInfo = vks::initializers::descriptorSetAllocateInfo(DescriptorPool, &DescriptorSetLayout, 1);
	VK_CHECK_RESULT(vkAllocateDescriptorSets(Device->logicalDevice, &SetAllocInfo, &DescriptorSet));
	std::vector<VkWriteDescriptorSet> WriteSet{
		vks::initializers::writeDescriptorSet(DescriptorSet,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0,&uboBuffer.descriptor)
	};
	vkUpdateDescriptorSets(Device->logicalDevice, 1, WriteSet.data(), 0, nullptr);

	//pipeline
	VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo();

	const std::vector< VkDescriptorSetLayout> SetLayouts = { DescriptorSetLayout,vkglTF::descriptorSetLayoutImage };
	PipelineLayoutCreateInfo.setLayoutCount = 2;
	PipelineLayoutCreateInfo.pSetLayouts = SetLayouts.data();

	VK_CHECK_RESULT(vkCreatePipelineLayout(Device->logicalDevice, &PipelineLayoutCreateInfo, nullptr, &PipelineLayout));

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, false);
	VkPipelineVertexInputStateCreateInfo* vertexInputState = vkglTF::Vertex::getPipelineVertexInputState({ vkglTF::VertexComponent::Position, vkglTF::VertexComponent::UV, vkglTF::VertexComponent::Color, vkglTF::VertexComponent::Normal });
	VkPipelineRasterizationStateCreateInfo rasterState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
	std::array<VkPipelineColorBlendAttachmentState,3> attachmentState = {
		vks::initializers::pipelineColorBlendAttachmentState(0xf,VK_FALSE),
		vks::initializers::pipelineColorBlendAttachmentState(0xf,VK_FALSE),
		vks::initializers::pipelineColorBlendAttachmentState(0xf,VK_FALSE)
	};
	VkPipelineColorBlendStateCreateInfo colorblendState = vks::initializers::pipelineColorBlendStateCreateInfo(3,attachmentState.data());
	VkPipelineDepthStencilStateCreateInfo depthstencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
	VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
	VkPipelineMultisampleStateCreateInfo multisampleState= vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
	std::vector<VkDynamicState> dynamicStates= {
		VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicSate = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStates.data(),2);
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
	shaderStages[0] = exampleBase->loadShader( getShaderPath() + "Geometry/gbuffer.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = exampleBase->loadShader( getShaderPath() + "Geometry/gbuffer.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = vks::initializers::pipelineCreateInfo(PipelineLayout, FrameBuffer->renderPass, 0);
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pVertexInputState = vertexInputState;
	pipelineCreateInfo.pRasterizationState = &rasterState;
	pipelineCreateInfo.pColorBlendState = &colorblendState;
	pipelineCreateInfo.pDepthStencilState = &depthstencilState;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pDynamicState = &dynamicSate;
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaderStages.data();
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(Device->logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &Pipeline));


	VkSemaphoreCreateInfo semaphoreCreateInfo = vks::initializers::semaphoreCreateInfo();
	VK_CHECK_RESULT(vkCreateSemaphore(Device->logicalDevice, &semaphoreCreateInfo, nullptr, &geometey));

	VkCommandBufferAllocateInfo cmdBufAllocateInfo = vks::initializers::commandBufferAllocateInfo(exampleBase->cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
	VK_CHECK_RESULT(vkAllocateCommandBuffers(Device->logicalDevice, &cmdBufAllocateInfo, &commandBuffer));
}

void GeometryPass::draw()
{
	VkCommandBufferBeginInfo cmdBeginInfo = vks::initializers::commandBufferBeginInfo();
	VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &cmdBeginInfo));

	// Clear values for all attachments written in the fragment shader
	std::vector<VkClearValue> clearValues(4);
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[3].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
	renderPassBeginInfo.renderPass = FrameBuffer->renderPass;
	renderPassBeginInfo.framebuffer = FrameBuffer->framebuffer;
	renderPassBeginInfo.renderArea.extent.width = FrameBuffer->width;
	renderPassBeginInfo.renderArea.extent.height = FrameBuffer->height;
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassBeginInfo.pClearValues = clearValues.data();


	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = vks::initializers::viewport((float)FrameBuffer->width, (float)FrameBuffer->height, 0.0f, 1.0f);
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor = vks::initializers::rect2D(FrameBuffer->width, FrameBuffer->height, 0, 0);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 1, &DescriptorSet, 0, nullptr);
	scene->draw(commandBuffer, vkglTF::RenderFlags::BindImages, PipelineLayout);

	vkCmdEndRenderPass(commandBuffer);

	VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
}

void GeometryPass::update()
{
	updateUboBuffer();
}

void GeometryPass::updateUboBuffer()
{
	uboMvp.model = glm::mat4(1.f);
	uboMvp.view = exampleBase->camera.matrices.view;
	uboMvp.projection = exampleBase->camera.matrices.perspective;

	uboBuffer.map();
	uboBuffer.copyTo(&uboMvp, sizeof(UBO));
	uboBuffer.unmap();
}
