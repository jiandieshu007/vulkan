#pragma once


#include "VulkanFrameBuffer.hpp"

#include "VulkanglTFModel.h"
#include <camera.hpp>

class RenderPass {
public:
	RenderPass(vks::VulkanDevice& device, Camera& camera) : Device(device), FrameBuffer(&Device) ,Camera(camera) {};



	virtual void prepare(VkQueue& queue) = 0;
	virtual void draw(VkCommandBuffer& cmdBuffer) = 0;
	virtual void update() = 0;
	void loadScene(const std::string& path, VkQueue& queue) {
		vkglTF::descriptorBindingFlags = vkglTF::DescriptorBindingFlags::ImageBaseColor;
		const uint32_t gltfLoadingFlags = vkglTF::FileLoadingFlags::FlipY | vkglTF::FileLoadingFlags::PreTransformVertices;
		scene.loadFromFile(path, &Device, queue, gltfLoadingFlags);
	};
public:
	vks::Framebuffer FrameBuffer;
	vkglTF::Model scene;

	vks::VulkanDevice Device{ VK_NULL_HANDLE };
	Camera Camera;

	VkDescriptorPool DescriptorPool{ VK_NULL_HANDLE };
	VkPipeline Pipeline{ VK_NULL_HANDLE };
	VkPipelineLayout PipelineLayout{ VK_NULL_HANDLE };
	VkDescriptorSet DescriptorSet{ VK_NULL_HANDLE };
	VkDescriptorSetLayout DescriptorSetLayout{ VK_NULL_HANDLE };
};