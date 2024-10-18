#pragma once

#include <queue>

#include "VulkanFrameBuffer.hpp"
#include "VulkanglTFModel.h"

#include "vulkan/vulkan_hpp_macros.hpp"

#include "vulkanexamplebase.h"


const std::string shaderDir = "shader/";


constexpr int LightCount = 1;

struct PointLight {
	alignas(16) glm::vec3 position;
	alignas(16) glm::vec3 intensity;
};

struct UBO {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
};

inline float randomFloat(float min, float max) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> dis(min, max);
	return static_cast<float>(dis(gen));
}


class RenderPass {
public:

	RenderPass(VulkanExampleBase* example) : exampleBase(example), Device(exampleBase->vulkanDevice)
	{
		
	}

	virtual ~RenderPass()
	{
		if (Pipeline != VK_NULL_HANDLE) {
			vkDestroyPipeline(Device->logicalDevice, Pipeline, nullptr);
			Pipeline = VK_NULL_HANDLE;
		}
		if (PipelineLayout != VK_NULL_HANDLE) {
			vkDestroyPipelineLayout(Device->logicalDevice, PipelineLayout, nullptr);
			PipelineLayout = VK_NULL_HANDLE;
		}
		if (DescriptorSetLayout != VK_NULL_HANDLE) {
			vkDestroyDescriptorSetLayout(Device->logicalDevice, DescriptorSetLayout, nullptr);
			DescriptorSetLayout = VK_NULL_HANDLE;
		}
		if (DescriptorPool != VK_NULL_HANDLE) {
			vkDestroyDescriptorPool(Device->logicalDevice, DescriptorPool, nullptr);
			DescriptorPool = VK_NULL_HANDLE;
		}
		if( FrameBuffer != VK_NULL_HANDLE) FrameBuffer->~Framebuffer();
		if (scene != nullptr) scene->~Model();
	};

	virtual void prepare() = 0;
	virtual void draw() = 0;
	virtual void update(VkQueue& queue) = 0;
	virtual void createRenderpass() = 0;
	void loadScene(const std::string& path, VkQueue& queue) {
		vkglTF::descriptorBindingFlags = vkglTF::DescriptorBindingFlags::ImageBaseColor;
		const uint32_t gltfLoadingFlags = vkglTF::FileLoadingFlags::FlipY | vkglTF::FileLoadingFlags::PreTransformVertices;
		scene->loadFromFile(path, Device, queue, gltfLoadingFlags);
	}

	std::string getShaderPath() const
	{
		return getShaderBasePath() + shaderDir;
	}
public:

	VulkanExampleBase* exampleBase {VK_NULL_HANDLE};
	vks::VulkanDevice* Device{ VK_NULL_HANDLE };
	vks::Framebuffer* FrameBuffer{VK_NULL_HANDLE};

	vkglTF::Model* scene {nullptr};

	VkDescriptorPool DescriptorPool{ VK_NULL_HANDLE };
	VkPipeline Pipeline{ VK_NULL_HANDLE };
	VkPipelineLayout PipelineLayout{ VK_NULL_HANDLE };
	VkDescriptorSet DescriptorSet{ VK_NULL_HANDLE };
	VkDescriptorSetLayout DescriptorSetLayout{ VK_NULL_HANDLE };
};


