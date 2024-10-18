#include "shadowPass.h"


void ShadowPass::prepare()
{
    initLights();

    //pipeline
    VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(nullptr,0);

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = 2*sizeof(glm::mat4);

    PipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    PipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

	VK_CHECK_RESULT(vkCreatePipelineLayout(Device->logicalDevice, &PipelineLayoutCreateInfo, nullptr, &PipelineLayout));

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, false);
	VkPipelineVertexInputStateCreateInfo* vertexInputState = vkglTF::Vertex::getPipelineVertexInputState({ vkglTF::VertexComponent::Position});
	VkPipelineRasterizationStateCreateInfo rasterState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);

	VkPipelineColorBlendStateCreateInfo colorblendState = vks::initializers::pipelineColorBlendStateCreateInfo(0, nullptr);
	VkPipelineDepthStencilStateCreateInfo depthstencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
	VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
	VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicSate = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStates.data(), 2);
	std::array<VkPipelineShaderStageCreateInfo, 1> shaderStages;
	shaderStages[0] = exampleBase->loadShader(getShaderPath() + "shadowOfpointLight/PointLightShadow.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);


	VkGraphicsPipelineCreateInfo pipelineCreateInfo = vks::initializers::pipelineCreateInfo(PipelineLayout, pointLights[0].framebuffer[0]->renderPass, 0);
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pVertexInputState = vertexInputState;
	pipelineCreateInfo.pRasterizationState = &rasterState;
	pipelineCreateInfo.pColorBlendState = &colorblendState;
	pipelineCreateInfo.pDepthStencilState = &depthstencilState;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pDynamicState = &dynamicSate;
	pipelineCreateInfo.stageCount = 1;
	pipelineCreateInfo.pStages = shaderStages.data();
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(Device->logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &Pipeline));

    VkCommandBufferAllocateInfo cmdBufAllocateInfo = vks::initializers::commandBufferAllocateInfo(exampleBase->cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
    VK_CHECK_RESULT(vkAllocateCommandBuffers(Device->logicalDevice, &cmdBufAllocateInfo, &commandBuffer));
}

void ShadowPass::draw()
{
    VkCommandBufferBeginInfo cmdBeginInfo = vks::initializers::commandBufferBeginInfo();
    VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &cmdBeginInfo));
    std::vector<VkClearValue> clearValue(1);
    clearValue[0].depthStencil = { 1,0 };

    for(int i=0; i<LightCount; ++i)
    {
	    for(int j =0; j<6; ++j)
	    {
            VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
            renderPassBeginInfo.renderPass = pointLights[i].framebuffer[j]->renderPass;
            renderPassBeginInfo.framebuffer = pointLights[i].framebuffer[j]->framebuffer;
            renderPassBeginInfo.renderArea.extent.width = depthsWidth;
            renderPassBeginInfo.renderArea.extent.height = depthsHeight;
            renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValue.size());
            renderPassBeginInfo.pClearValues = clearValue.data();

            vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo,VK_SUBPASS_CONTENTS_INLINE);

            VkViewport viewport = vks::initializers::viewport((float)depthsWidth, (float)depthsHeight, 0.0f, 1.0f);
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor = vks::initializers::rect2D(depthsWidth, depthsHeight, 0, 0);
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);

            vkCmdPushConstants(commandBuffer, PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &pointLights[i].model);

	    	vkCmdPushConstants(commandBuffer, PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), sizeof(glm::mat4), &pointLights[i].view[j]);
            gPass->scene->draw(commandBuffer);

	    	vkCmdEndRenderPass(commandBuffer);
	    }
    }

    vkEndCommandBuffer(commandBuffer);
}

void ShadowPass::update(VkQueue& queue)
{

}

void ShadowPass::createRenderpass()
{

}

void ShadowPass::initLights()
{
    // projection matrix
    glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, nearPlane, farPlane);

    std::vector<glm::vec3> poss{ glm::vec3(5,-3,0), glm::vec3(10,0,-3)};
    pointLights.resize(LightCount);
    for (int i = 0; i < LightCount; ++i)
    {
        auto& tmp = pointLights[i];

        PointLight light;
        light.position = poss[i];
        light.intensity = glm::vec3(0.5);

        tmp.model = glm::translate(glm::mat4(1.0f), -light.position);
        tmp.Light = light;
        for(int j=0; j<6; ++j)
        {
            tmp.framebuffer[j] = new vks::Framebuffer(exampleBase->vulkanDevice);
            tmp.view[j] = shadowProj * glm::lookAt(light.position, light.position + directions[j], upVectors[j]);
            initShadowFramebuffer(tmp.framebuffer[j]);
        }
    }
}

void ShadowPass::initShadowFramebuffer(vks::Framebuffer* framebuffer)
{
    framebuffer->width = depthsWidth;
    framebuffer->height = depthsHeight;

    // 
    vks::AttachmentCreateInfo attachmentInfo = {};
    attachmentInfo.width = depthsWidth;
    attachmentInfo.height = depthsHeight;
    attachmentInfo.layerCount = 1;
    attachmentInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    VkFormat attDepthFormat;
    VkBool32 validDepthFormat = vks::tools::getSupportedDepthFormat(Device->physicalDevice, &attDepthFormat);
    assert(validDepthFormat);

    attachmentInfo.format = attDepthFormat;
    framebuffer->addAttachment(attachmentInfo);
    framebuffer->createSampler(VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    framebuffer->createRenderPass(true);

}
