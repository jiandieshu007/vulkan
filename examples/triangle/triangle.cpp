#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"


struct Vertex {
    glm::vec3 postion;
	glm::vec3 normal;
    glm::vec2 uv;
};

class VulkanExample : public VulkanExampleBase {
public:
    struct UniformData {
        glm::mat4 projection;
        glm::mat4 modelview;
    }uniforms;
    vks::Buffer UniformBuffer;
    
    vks::Buffer VertexBuffer, IndexBuffer;
    uint32_t index{ 0 };

    vks::Texture2D texture;

    VkPipeline pipeline{ VK_NULL_HANDLE };
    VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
    VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
    VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };


    VulkanExample() : VulkanExampleBase() {
        title = "First pass";
        camera.type = Camera::CameraType::firstperson;
        camera.movementSpeed = 5.0f;
#ifndef __ANDROID__
        camera.rotationSpeed = 0.25f;
#endif
        camera.position = { 2.15f, 0.3f, -8.75f };
        camera.setRotation(glm::vec3(-0.75f, 12.5f, 0.0f));
        camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 256.0f);
    }
    ~VulkanExample()
    {
        if (device)
        {
            UniformBuffer.destroy();
            IndexBuffer.destroy();
            VertexBuffer.destroy();
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
            
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
            vkDestroyPipeline(device, pipeline, nullptr);
        }
    }
    void createModel() {

        std::vector<Vertex> vertices{
            { {  1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f },{0,0} },
            { { -1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f },{0,1} },
            { { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } ,{1,1}}
        };
        std::vector<uint32_t> indices{ 0,1,2 };

        vks::Buffer stagingBufferV, stagingBufferI;
        vulkanDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBufferV, sizeof(Vertex) * vertices.size(), vertices.data());
        vulkanDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBufferI, sizeof(uint32_t) * indices.size(), indices.data());

        vulkanDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &VertexBuffer, sizeof(Vertex) * vertices.size());
        vulkanDevice->createBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &IndexBuffer, sizeof(uint32_t) * indices.size());

        vulkanDevice->copyBuffer(&stagingBufferV, &VertexBuffer, queue);
        vulkanDevice->copyBuffer(&stagingBufferI, &IndexBuffer, queue);

        stagingBufferV.destroy();
        stagingBufferI.destroy();
    }

    void createUniforms()
    {
        vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &UniformBuffer, sizeof(UniformData), &uniforms);
        UniformBuffer.map();
        updateUniforms();
    }
    void createTexture()
    {
        texture.loadFromFile(getAssetPath() + "textures/vulkan_cloth_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice,queue);
    }
    void updateUniforms()
    {
        uniforms.modelview = camera.matrices.view;
        uniforms.projection = camera.matrices.perspective;
        memcpy(UniformBuffer.mapped, &uniforms, sizeof(uniforms));
    }
    void createDescriptorSet()
    {
        std::vector<VkDescriptorPoolSize> PS{
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,2},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,2}
        };
        VkDescriptorPoolCreateInfo PC = vks::initializers::descriptorPoolCreateInfo(PS, 2);
        vkCreateDescriptorPool(device, &PC, nullptr, &descriptorPool);

        std::vector<VkDescriptorSetLayoutBinding> SLB{
            vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,VK_SHADER_STAGE_VERTEX_BIT,0),
            vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,VK_SHADER_STAGE_FRAGMENT_BIT,1)
        };

        VkDescriptorSetLayoutCreateInfo DSLC = vks::initializers::descriptorSetLayoutCreateInfo(SLB.data(), 2);
        vkCreateDescriptorSetLayout(device, &DSLC, nullptr, &descriptorSetLayout);
        VkDescriptorSetAllocateInfo DSAI = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
        vkAllocateDescriptorSets(device, &DSAI, &descriptorSet);

        std::vector<VkWriteDescriptorSet> writeDescriptors{
            vks::initializers::writeDescriptorSet(descriptorSet,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0,&UniformBuffer.descriptor),
            vks::initializers::writeDescriptorSet(descriptorSet,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,&texture.descriptor)
        };
        vkUpdateDescriptorSets(device, 2, writeDescriptors.data(), 0, nullptr);
    }
    void createPipeline()
    {
        VkPipelineLayoutCreateInfo PLCI = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
        vkCreatePipelineLayout(device, &PLCI, nullptr, &pipelineLayout);
        // Vertex input state
        std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
            vks::initializers::vertexInputBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX)
        };
        std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
            vks::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, postion)),
            vks::initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)),
            vks::initializers::vertexInputAttributeDescription(0,2,VK_FORMAT_R32G32_SFLOAT,offsetof(Vertex,uv))
        };
        VkPipelineVertexInputStateCreateInfo vertexInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
        vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
        vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
        vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
        vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

        VkPipelineInputAssemblyStateCreateInfo input_assembly = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, false);
        VkPipelineRasterizationStateCreateInfo rasterization_state = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE, 0);
        VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
        VkPipelineColorBlendStateCreateInfo color_blend = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
        VkPipelineDepthStencilStateCreateInfo depth_stencil_state = vks::initializers::pipelineDepthStencilStateCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
        VkPipelineViewportStateCreateInfo views_create_info = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
        VkPipelineMultisampleStateCreateInfo multisample_state = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
        std::vector<VkDynamicState> dynamic_states{ VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamic_state = vks::initializers::pipelineDynamicStateCreateInfo(dynamic_states);
        std::array<VkPipelineShaderStageCreateInfo, 2> shader_stage;
        shader_stage[0] = loadShader(getShadersPath() + "nine/triangle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        shader_stage[1] = loadShader(getShadersPath() + "nine/triangle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

        VkGraphicsPipelineCreateInfo pipeline_create = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass, 0);
        pipeline_create.pVertexInputState = &vertexInputState;
        pipeline_create.pInputAssemblyState = &input_assembly;
        pipeline_create.pRasterizationState = &rasterization_state;
        pipeline_create.pColorBlendState = &color_blend;
        pipeline_create.pMultisampleState = &multisample_state;
        pipeline_create.pDepthStencilState = &depth_stencil_state;
        pipeline_create.pViewportState = &views_create_info;
        pipeline_create.pDynamicState = &dynamic_state;
        pipeline_create.stageCount = static_cast<uint32_t>(shader_stage.size());
        pipeline_create.pStages = shader_stage.data();

        vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create, nullptr, &pipeline);
    }
    void createCommandBuffer()
    {
        VkCommandBufferBeginInfo cmdBufBeginInfo = vks::initializers::commandBufferBeginInfo();

        VkClearValue clearColor[2];
        clearColor[0].color = { 0,0,0,0 };
        clearColor[1].depthStencil = { 1,0 };

        VkRenderPassBeginInfo renderpassBeginInfo = vks::initializers::renderPassBeginInfo();
        renderpassBeginInfo.renderPass = renderPass;
        renderpassBeginInfo.renderArea.offset.x = 0;
        renderpassBeginInfo.renderArea.offset.y = 0;
        renderpassBeginInfo.renderArea.extent.width = width;
        renderpassBeginInfo.renderArea.extent.height = height;
        renderpassBeginInfo.clearValueCount = 2;
        renderpassBeginInfo.pClearValues = clearColor;

        for (uint32_t i = 0; i < drawCmdBuffers.size(); ++i)
        {
            renderpassBeginInfo.framebuffer = frameBuffers[i];

            vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufBeginInfo);
            vkCmdBeginRenderPass(drawCmdBuffers[i], &renderpassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

            VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
            vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

            VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
            vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

            vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

            vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            VkDeviceSize offset[1]{ 0 };
            vkCmdBindVertexBuffers(drawCmdBuffers[i], 0, 1, &VertexBuffer.buffer, offset);
            vkCmdBindIndexBuffer(drawCmdBuffers[i], IndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        	vkCmdDrawIndexed(drawCmdBuffers[i], 3, 1, 0, 0,0);

            vkCmdEndRenderPass(drawCmdBuffers[i]);

            vkEndCommandBuffer(drawCmdBuffers[i]);
        }
    }
    void draw()
    {
        VulkanExampleBase::prepareFrame();
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
        VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
        VulkanExampleBase::submitFrame();
    }

    void prepare() override
    {
        VulkanExampleBase::prepare();
        createModel();
        createTexture();
        createUniforms();
        createDescriptorSet();
        createPipeline();
        createCommandBuffer();
        prepared = true;
    }
    virtual void render()
    {
        if (!prepared)
            return;
        updateUniforms();
        draw();
    }
};



VULKAN_EXAMPLE_MAIN()