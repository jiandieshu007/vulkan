#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"
#include "vulkan/vulkan_hpp_macros.hpp"


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

    vkglTF::Model Scene;

    vks::Buffer VertexBuffer, IndexBuffer;
    uint32_t index{ 0 };

    vks::Texture2D texture;

    

    struct
    {
        VkPipeline scene;
        VkPipeline display;
    }pipelines;

    struct
    {
        VkPipelineLayout scene;
        VkPipelineLayout display;
    }pipelineLayouts;

    struct 
    {
        VkDescriptorSet scenes;
        VkDescriptorSet display;
    }descriptorSets;

    struct 
    {
        VkDescriptorSetLayout scene;
        VkDescriptorSetLayout display;
    }descriptorSetLayouts;

    // G-Buffer framebuffer attachments
    struct FrameBufferAttachment {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory mem = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkFormat format;
    };
    struct Attachments {
        FrameBufferAttachment position, normal, albedo;
        int32_t width;
        int32_t height;
    } attachments;


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
    void clearAttachment(FrameBufferAttachment* attachment)
    {
        vkDestroyImageView(device, attachment->view, nullptr);
        vkDestroyImage(device, attachment->image, nullptr);
        vkFreeMemory(device, attachment->mem, nullptr);
    }

    // Create a frame buffer attachment
    void createAttachment(VkFormat format, VkImageUsageFlags usage, FrameBufferAttachment* attachment)
    {
        if (attachment->image != VK_NULL_HANDLE) {
            clearAttachment(attachment);
        }

        VkImageAspectFlags aspectMask = 0;

        attachment->format = format;

        if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
        {
            aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }
        if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        assert(aspectMask > 0);

        VkImageCreateInfo image = vks::initializers::imageCreateInfo();
        image.imageType = VK_IMAGE_TYPE_2D;
        image.format = format;
        image.extent.width = attachments.width;
        image.extent.height = attachments.height;
        image.extent.depth = 1;
        image.mipLevels = 1;
        image.arrayLayers = 1;
        image.samples = VK_SAMPLE_COUNT_1_BIT;
        image.tiling = VK_IMAGE_TILING_OPTIMAL;
        // VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT flag is required for input attachments
        image.usage = usage | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
        VkMemoryRequirements memReqs;

        VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &attachment->image));
        vkGetImageMemoryRequirements(device, attachment->image, &memReqs);
        memAlloc.allocationSize = memReqs.size;
        memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &attachment->mem));
        VK_CHECK_RESULT(vkBindImageMemory(device, attachment->image, attachment->mem, 0));

        VkImageViewCreateInfo imageView = vks::initializers::imageViewCreateInfo();
        imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageView.format = format;
        imageView.subresourceRange = {};
        imageView.subresourceRange.aspectMask = aspectMask;
        imageView.subresourceRange.baseMipLevel = 0;
        imageView.subresourceRange.levelCount = 1;
        imageView.subresourceRange.baseArrayLayer = 0;
        imageView.subresourceRange.layerCount = 1;
        imageView.image = attachment->image;
        VK_CHECK_RESULT(vkCreateImageView(device, &imageView, nullptr, &attachment->view));
    }

    void CreateGbufferAttachment()
    {
        createAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &attachments.position);	// (World space) Positions
        createAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &attachments.normal);		// (World space) Normals
        createAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &attachments.albedo);			// Albedo (color)
    }
    void setupRenderPass() override
    {
        attachments.width = width;
        attachments.height = height;

        CreateGbufferAttachment();

        std::array<VkAttachmentDescription, 5> attachments{};

        // Color attachment
        attachments[0].format = swapChain.colorFormat;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // Deferred attachments
        // Position
        attachments[1].format = this->attachments.position.format;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        // Normals
        attachments[2].format = this->attachments.normal.format;
        attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        // Albedo
        attachments[3].format = this->attachments.albedo.format;
        attachments[3].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[3].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[3].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        // Depth attachment
        attachments[4].format = depthFormat;
        attachments[4].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[4].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[4].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[4].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[4].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[4].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[4].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        std::array<VkSubpassDescription, 2> subpassDescriptions{};

        // first subpass

        VkAttachmentReference colorRefrences[3];

        colorRefrences[0] = { 1,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
        colorRefrences[1] = { 2,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
        colorRefrences[2] = { 3,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        VkAttachmentReference depthRefrence{ 4,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

        subpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescriptions[0].colorAttachmentCount = 3;
        subpassDescriptions[0].pColorAttachments = colorRefrences;
        subpassDescriptions[0].pDepthStencilAttachment = &depthRefrence;

        //second subpass
        VkAttachmentReference colorRefrence{ 0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        VkAttachmentReference inputRefrences[3];

        inputRefrences[0] = { 1,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        inputRefrences[1] = { 2,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        inputRefrences[2] = { 3,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

        subpassDescriptions[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescriptions[1].colorAttachmentCount = 1;
        subpassDescriptions[1].pColorAttachments = &colorRefrence;
        subpassDescriptions[1].pDepthStencilAttachment = &depthRefrence;
        subpassDescriptions[1].inputAttachmentCount = 3;
        subpassDescriptions[1].pInputAttachments = inputRefrences;

        //subpass dependency

        std::array<VkSubpassDependency, 4> subpassDependencies;

        subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        subpassDependencies[0].dstSubpass = 0;
        subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDependencies[0].srcAccessMask = 0;
        subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        subpassDependencies[0].dependencyFlags = 0;

        subpassDependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
        subpassDependencies[1].dstSubpass = 0;
        subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;;
        subpassDependencies[1].srcAccessMask = 0;
        subpassDependencies[1].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        subpassDependencies[1].dependencyFlags = 0;

        subpassDependencies[2].srcSubpass = 0;
        subpassDependencies[2].dstSubpass = 1;
        subpassDependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDependencies[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        subpassDependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        subpassDependencies[2].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        subpassDependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        subpassDependencies[3].srcSubpass = 1;
        subpassDependencies[3].dstSubpass = VK_SUBPASS_EXTERNAL;
        subpassDependencies[3].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDependencies[3].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        subpassDependencies[3].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        subpassDependencies[3].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        subpassDependencies[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo renderpassInfo = vks::initializers::renderPassCreateInfo();
        renderpassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderpassInfo.pAttachments = attachments.data();
        renderpassInfo.subpassCount = static_cast<uint32_t>(subpassDescriptions.size());
        renderpassInfo.pSubpasses = subpassDescriptions.data();
        renderpassInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
        renderpassInfo.pDependencies = subpassDependencies.data();

        vkCreateRenderPass(device, &renderpassInfo, nullptr, &renderPass);
    }

    void setupFrameBuffer() override
    {
	    if( attachments.width!=width || attachments.height != height)
	    {
            attachments.width = width;
            attachments.height = height;
            CreateGbufferAttachment();

            std::vector<VkDescriptorImageInfo> descriptorImageInfos{
                vks::initializers::descriptorImageInfo(VK_NULL_HANDLE,attachments.position.view,VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL),
                vks::initializers::descriptorImageInfo(VK_NULL_HANDLE,attachments.normal.view,VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL),
                vks::initializers::descriptorImageInfo(VK_NULL_HANDLE,attachments.albedo.view,VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL)
            };
            std::vector<VkWriteDescriptorSet> writeDescriptorsets;
            for(uint32_t i=0; i<descriptorImageInfos.size(); ++i)
            {
                writeDescriptorsets.push_back(vks::initializers::writeDescriptorSet(descriptorSets.display, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, i, &descriptorImageInfos[i]));
            }
            vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorsets.size()), writeDescriptorsets.data(), 0, nullptr);
	    }

        VkImageView attachments[5];

        VkFramebufferCreateInfo framebufferCI = vks::initializers::framebufferCreateInfo();

        framebufferCI.width = width;
        framebufferCI.height = height;
        framebufferCI.layers = 1;
        framebufferCI.renderPass = renderPass;
        framebufferCI.attachmentCount = 5;
        framebufferCI.pAttachments = attachments;

        frameBuffers.resize(swapChain.imageCount);
        for(uint32_t i=0; i<swapChain.imageCount; ++i)
        {
            attachments[0] = swapChain.buffers[i].view;
            attachments[1] = this->attachments.position.view;
            attachments[2] = this->attachments.normal.view;
            attachments[3] = this->attachments.albedo.view;
            attachments[4] = depthStencil.view;

            vkCreateFramebuffer(device, &framebufferCI, nullptr, &frameBuffers[i]);
        }

    }

    void LoadAsset()
    {
        const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY;
        Scene.loadFromFile(getAssetPath() + "models/chinesedragon.gltf", vulkanDevice, queue, glTFLoadingFlags);

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
        std::vector<VkDescriptorPoolSize> descriptorPoolSize{
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,3},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,3}

        };
        VkDescriptorPoolCreateInfo descriptorPoolCI = vks::initializers::descriptorPoolCreateInfo(descriptorPoolSize, 2);
        vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr, &descriptorPool);

        // first pass
        std::vector<VkDescriptorSetLayoutBinding> SLB{
            vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,VK_SHADER_STAGE_VERTEX_BIT,0),
        };

        VkDescriptorSetLayoutCreateInfo DSLC = vks::initializers::descriptorSetLayoutCreateInfo(SLB.data(), 1);
        vkCreateDescriptorSetLayout(device, &DSLC, nullptr, &descriptorSetLayouts.scene);
        VkDescriptorSetAllocateInfo DSAI = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayouts.scene, 1);
        vkAllocateDescriptorSets(device, &DSAI, &descriptorSets.scenes);

        std::vector<VkWriteDescriptorSet> writeDescriptors{
            vks::initializers::writeDescriptorSet(descriptorSets.scenes,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0,&UniformBuffer.descriptor)
        };
        vkUpdateDescriptorSets(device, 1, writeDescriptors.data(), 0, nullptr);

        // second subpass

        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{
            vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,VK_SHADER_STAGE_FRAGMENT_BIT,0),
            vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,VK_SHADER_STAGE_FRAGMENT_BIT,1),
            vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,VK_SHADER_STAGE_FRAGMENT_BIT,2)
        };

        VkDescriptorSetLayoutCreateInfo descripotrLayoutCI = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), 
            static_cast<uint32_t>(setLayoutBindings.size()));

        vkCreateDescriptorSetLayout(device, &descripotrLayoutCI, nullptr, &descriptorSetLayouts.display);

        VkDescriptorSetAllocateInfo descriptorLayoutAlloc = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayouts.display, 1);
        vkAllocateDescriptorSets(device, &descriptorLayoutAlloc, &descriptorSets.display);

        auto texDescriptorPos = vks::initializers::descriptorImageInfo(nullptr, attachments.position.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        auto texDescriptorNor = vks::initializers::descriptorImageInfo(nullptr, attachments.normal.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        auto texDescriptorAlbedo = vks::initializers::descriptorImageInfo(nullptr, attachments.albedo.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        std::vector<VkWriteDescriptorSet> writeDescriptor{
            vks::initializers::writeDescriptorSet(descriptorSets.display,VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,0,&texDescriptorPos),
            vks::initializers::writeDescriptorSet(descriptorSets.display,VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, &texDescriptorNor),
            vks::initializers::writeDescriptorSet(descriptorSets.display,VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,2,&texDescriptorAlbedo)
        };

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptor.size()), writeDescriptor.data(), 0, nullptr);

    }
    void createPipeline()
    {



        VkPipelineLayoutCreateInfo PLCI = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayouts.scene, 1);
        vkCreatePipelineLayout(device, &PLCI, nullptr, &pipelineLayouts.scene);

        VkGraphicsPipelineCreateInfo pipeline_create = vks::initializers::pipelineCreateInfo(pipelineLayouts.scene, renderPass);

        VkPipelineVertexInputStateCreateInfo* vertexInputState = vkglTF::Vertex::getPipelineVertexInputState({ vkglTF::VertexComponent::Position,vkglTF::VertexComponent::Color,vkglTF::VertexComponent::Normal });
        VkPipelineInputAssemblyStateCreateInfo input_assembly = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, false);
        VkPipelineRasterizationStateCreateInfo rasterization_state = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE, 0);
    	std::array<VkPipelineColorBlendAttachmentState,3> blendAttachments{
            vks::initializers::pipelineColorBlendAttachmentState(0xf,VK_FALSE),
            vks::initializers::pipelineColorBlendAttachmentState(0xf,VK_FALSE),
            vks::initializers::pipelineColorBlendAttachmentState(0xf,VK_FALSE)
        };
        VkPipelineColorBlendStateCreateInfo blendState = vks::initializers::pipelineColorBlendStateCreateInfo(3, blendAttachments.data());
        VkPipelineDepthStencilStateCreateInfo depth_stencil_state = vks::initializers::pipelineDepthStencilStateCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
        VkPipelineViewportStateCreateInfo views_create_info = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
        VkPipelineMultisampleStateCreateInfo multisample_state = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
        std::vector<VkDynamicState> dynamic_states{ VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamic_state = vks::initializers::pipelineDynamicStateCreateInfo(dynamic_states);
        std::array<VkPipelineShaderStageCreateInfo, 2> shader_stage;
        shader_stage[0] = loadShader(getShadersPath() + "nine/gbuffer.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        shader_stage[1] = loadShader(getShadersPath() + "nine/gbuffer.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);



        pipeline_create.pInputAssemblyState = &input_assembly;
        pipeline_create.pVertexInputState = vertexInputState;
        pipeline_create.pColorBlendState = &blendState;
        pipeline_create.pRasterizationState = &rasterization_state;
        pipeline_create.pMultisampleState = &multisample_state;
        pipeline_create.pDepthStencilState = &depth_stencil_state;
        pipeline_create.pViewportState = &views_create_info;
        pipeline_create.pDynamicState = &dynamic_state;
        pipeline_create.stageCount = static_cast<uint32_t>(shader_stage.size());
        pipeline_create.pStages = shader_stage.data();
        pipeline_create.subpass = 0;

        vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create, nullptr, &pipelines.scene);


        VkPipelineLayoutCreateInfo DisplayPipelineLayout = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayouts.display, 1);
        vkCreatePipelineLayout(device, &DisplayPipelineLayout, nullptr, &pipelineLayouts.display);
        pipeline_create = vks::initializers::pipelineCreateInfo(pipelineLayouts.display , renderPass);


        VkPipelineVertexInputStateCreateInfo emptyInputState{};
        emptyInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
        VkPipelineColorBlendStateCreateInfo color_blend = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
        std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;
        shader_stages[0] = loadShader(getShadersPath() + "nine/display.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        shader_stages[1] = loadShader(getShadersPath() + "nine/display.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);


        pipeline_create.pVertexInputState = &emptyInputState;
        pipeline_create.pInputAssemblyState = &input_assembly;
        pipeline_create.pColorBlendState = &color_blend;        pipeline_create.pRasterizationState = &rasterization_state;
        pipeline_create.pMultisampleState = &multisample_state;
        pipeline_create.pDepthStencilState = &depth_stencil_state;
        pipeline_create.pViewportState = &views_create_info;
        pipeline_create.pDynamicState = &dynamic_state;
        pipeline_create.stageCount = static_cast<uint32_t>(shader_stages.size());
        pipeline_create.pStages = shader_stages.data();
        pipeline_create.subpass = 1;

        vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_create, nullptr, &pipelines.display);

    }

    void createCommandBuffer()
    {
        VkCommandBufferBeginInfo cmdBufBeginInfo = vks::initializers::commandBufferBeginInfo();

        VkClearValue clearColor[5];
        clearColor[0].color = { 0,0,0,0 };
        clearColor[1].color = { 0,0,0,0 };
        clearColor[2].color = { 0,0,0,0 };
        clearColor[3].color = { 0,0,0,0 };
        clearColor[4].depthStencil = { 1,0 };

        VkRenderPassBeginInfo renderpassBeginInfo = vks::initializers::renderPassBeginInfo();
        renderpassBeginInfo.renderPass = renderPass;
        renderpassBeginInfo.renderArea.offset.x = 0;
        renderpassBeginInfo.renderArea.offset.y = 0;
        renderpassBeginInfo.renderArea.extent.width = width;
        renderpassBeginInfo.renderArea.extent.height = height;
        renderpassBeginInfo.clearValueCount = 5;
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

            // first subpass
	        {
		        vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.scene, 0, 1, &descriptorSets.scenes, 0, nullptr);

            	vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.scene);
            	Scene.draw(drawCmdBuffers[i]);
	        }

            //second subpass

            {
                vkCmdNextSubpass(drawCmdBuffers[i], VK_SUBPASS_CONTENTS_INLINE);
                vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.display, 0, 1, &descriptorSets.display, 0, nullptr);
                vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.display);
                vkCmdDraw(drawCmdBuffers[i], 3, 1, 0, 0);
            }

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
        LoadAsset();
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