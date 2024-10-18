#include "lightPass.h"

void LightPass::prepare()
{
    prepareLight();

    std::vector<VkDescriptorPoolSize> poolSizes = {
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1),
		vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3+1)
    };
    auto PoolCreateInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 1);
    VK_CHECK_RESULT(vkCreateDescriptorPool(Device->logicalDevice, &PoolCreateInfo, nullptr, &DescriptorPool));

    std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,VK_SHADER_STAGE_FRAGMENT_BIT,0),
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,VK_SHADER_STAGE_FRAGMENT_BIT,1),
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,VK_SHADER_STAGE_FRAGMENT_BIT,2),
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,VK_SHADER_STAGE_FRAGMENT_BIT,3),
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,VK_SHADER_STAGE_FRAGMENT_BIT,4,1)
    };
    auto descriptorSetLayoutCreateInfo = vks::initializers::descriptorSetLayoutCreateInfo(descriptorSetLayoutBindings.data(), descriptorSetLayoutBindings.size());
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(Device->logicalDevice, &descriptorSetLayoutCreateInfo, nullptr, &DescriptorSetLayout));

    auto descriptorSetAlloc = vks::initializers::descriptorSetAllocateInfo(DescriptorPool, &DescriptorSetLayout, 1);
    VK_CHECK_RESULT(vkAllocateDescriptorSets(Device->logicalDevice, &descriptorSetAlloc, &DescriptorSet));

    auto sampler = geometryPass->FrameBuffer->sampler;
    auto depthSampler = shadowPass->pointLights[0].framebuffer[0]->sampler;
    std::vector<VkWriteDescriptorSet> WriteSet{
        vks::initializers::writeDescriptorSet(DescriptorSet,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,0,
            &vks::initializers::descriptorImageInfo(sampler,geometryPass->FrameBuffer->attachments[0].view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)),
        vks::initializers::writeDescriptorSet(DescriptorSet,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,
            &vks::initializers::descriptorImageInfo(sampler,geometryPass->FrameBuffer->attachments[1].view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)),
        vks::initializers::writeDescriptorSet(DescriptorSet,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,2,
            &vks::initializers::descriptorImageInfo(sampler,geometryPass->FrameBuffer->attachments[2].view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)),
        vks::initializers::writeDescriptorSet(DescriptorSet,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,3,
            &vks::initializers::descriptorImageInfo(depthSampler,DepthsCubeArray.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)),

        vks::initializers::writeDescriptorSet(DescriptorSet,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,4, &lightBuffer.descriptor)
    };
    vkUpdateDescriptorSets(Device->logicalDevice, WriteSet.size(), WriteSet.data(), 0, nullptr);

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::vec3);


    auto pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&DescriptorSetLayout);
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
    VK_CHECK_RESULT(vkCreatePipelineLayout(Device->logicalDevice, &pipelineLayoutCreateInfo, nullptr, &PipelineLayout));

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
    VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
    VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
    VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
    VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
    VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
    std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = vks::initializers::pipelineCreateInfo(PipelineLayout, exampleBase->renderPass, 0);
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineCreateInfo.pRasterizationState = &rasterizationState;
    pipelineCreateInfo.pColorBlendState = &colorBlendState;
    pipelineCreateInfo.pMultisampleState = &multisampleState;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pDepthStencilState = &depthStencilState;
    pipelineCreateInfo.pDynamicState = &dynamicState;
    pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCreateInfo.pStages = shaderStages.data();

    // Empty vertex input state for fullscreen passes
    VkPipelineVertexInputStateCreateInfo emptyVertexInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
    pipelineCreateInfo.pVertexInputState = &emptyVertexInputState;
    rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;

    // Final composition pipeline
    shaderStages[0] = exampleBase->loadShader(getShaderPath() + "Lighting/fullscreen.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = exampleBase->loadShader(getShaderPath() + "Lighting/composition.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(Device->logicalDevice, nullptr, 1, &pipelineCreateInfo, nullptr, &Pipeline));
}

void LightPass::update(VkQueue& queue)
{
    updatePointLightDepthsimage(queue, shadowPass->pointLights, &DepthsCubeArray);
}

void LightPass::draw()
{
    for (int32_t i = 0; i < exampleBase->drawCmdBuffers.size(); ++i)
    {
        VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

        VK_CHECK_RESULT(vkBeginCommandBuffer(exampleBase->drawCmdBuffers[i], &cmdBufInfo));



        // srcImage is VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL

    	// dstImage is VK_IMAGE_LAYOUT_UNDEFINED

        // so insert a barrier

        VkImageMemoryBarrier imageMemoryBarrier = {};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;  
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.image = DepthsCubeArray.image;          
        imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
        imageMemoryBarrier.subresourceRange.levelCount = 1;
        imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
        imageMemoryBarrier.subresourceRange.layerCount = 6 * LightCount;

        imageMemoryBarrier.srcAccessMask = VK_ACCESS_NONE; 
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;  
        
        vkCmdPipelineBarrier(
            exampleBase->drawCmdBuffers[i],
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,   
            VK_PIPELINE_STAGE_TRANSFER_BIT,      
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier);



        for (uint32_t k = 0; k < LightCount; ++k)
        {
            for (int j = 0; j < 6; ++j) {
                VkImageCopy copyRegion{};
                copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                copyRegion.srcSubresource.mipLevel = 0;
                copyRegion.srcSubresource.baseArrayLayer = 0;
                copyRegion.srcSubresource.layerCount = 1;
                copyRegion.srcOffset = { 0,0,0 };

                copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                copyRegion.dstSubresource.mipLevel = 0;
                copyRegion.dstSubresource.baseArrayLayer = k* 6 + j;
                copyRegion.dstSubresource.layerCount = 1;
                copyRegion.dstOffset = { 0,0,0 };

                copyRegion.extent.width = shadowPass->depthsWidth;
                copyRegion.extent.height = shadowPass->depthsHeight;
                copyRegion.extent.depth = 1;

                vkCmdCopyImage(exampleBase->drawCmdBuffers[i], shadowPass->pointLights[k].framebuffer[j]->attachments[0].image, VK_IMAGE_LAYOUT_GENERAL,
                    DepthsCubeArray.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
            }
        }

        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            exampleBase->drawCmdBuffers[i],
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier);

        std::vector<VkClearValue> clearValues(2);
        clearValues[0].color = exampleBase->defaultClearColor;
        clearValues[1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
        renderPassBeginInfo.renderPass = exampleBase->renderPass;
        renderPassBeginInfo.framebuffer = exampleBase->frameBuffers[i];
        renderPassBeginInfo.renderArea.extent.width = exampleBase->width;
        renderPassBeginInfo.renderArea.extent.height = exampleBase->height;
        renderPassBeginInfo.clearValueCount = 2;
        renderPassBeginInfo.pClearValues = clearValues.data();



        vkCmdBeginRenderPass(exampleBase->drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);


        VkViewport viewport = vks::initializers::viewport((float)exampleBase->width, (float)exampleBase->height, 0.0f, 1.0f);
        vkCmdSetViewport(exampleBase->drawCmdBuffers[i], 0, 1, &viewport);

        VkRect2D scissor = vks::initializers::rect2D(exampleBase->width, exampleBase->height, 0, 0);
        vkCmdSetScissor(exampleBase->drawCmdBuffers[i], 0, 1, &scissor);

        vkCmdBindDescriptorSets(exampleBase->drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 1, &DescriptorSet, 0, NULL);


        vkCmdBindPipeline(exampleBase->drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);

        vkCmdPushConstants(exampleBase->drawCmdBuffers[i], PipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::vec3), &exampleBase->camera.viewPos);

        vkCmdDraw(exampleBase->drawCmdBuffers[i], 3, 1, 0, 0);



        vkCmdEndRenderPass(exampleBase->drawCmdBuffers[i]);

        vkEndCommandBuffer(exampleBase->drawCmdBuffers[i]);
    }
}

void LightPass::createRenderpass()
{

}

void LightPass::prepareLight()
{

    createPointLightDepthsimage(shadowPass->pointLights[0], &DepthsCubeArray);


    for(int i=0; i<LightCount; ++i)
    {
        pointLights.push_back(shadowPass->pointLights[i].Light);
    }
    Device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &lightBuffer, sizeof(PointLight) * LightCount);
    lightBuffer.map();
    lightBuffer.copyTo(pointLights.data(), sizeof(PointLight) * LightCount);
    lightBuffer.unmap();
}

void LightPass::updateLight()
{

}
