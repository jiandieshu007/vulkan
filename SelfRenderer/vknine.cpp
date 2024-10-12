#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"
#include "vulkan/vulkan_hpp_macros.hpp"

#include "./RenderPass/Geometry.h"
#include "./RenderPass/lightPass.h"
class VulkanExample : public VulkanExampleBase {
public:
    VulkanExample() : VulkanExampleBase() {
        title = "First pass";
        camera.type = Camera::CameraType::firstperson;
        camera.movementSpeed = 5.0f;
#ifndef __ANDROID__
        camera.rotationSpeed = 0.25f;
#endif
        camera.position = { 1.0f, 0.75f, 0.0f };
        camera.setRotation(glm::vec3(0.0f, 90.0f, 0.0f));
        camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 256.0f);

    }
    ~VulkanExample()
    {
        gPass->~GeometryPass();
        lightPass->~LightPass();
        vkDestroyFence(device, fenceforQueue, nullptr);
    }
    // Enable physical device features required for this example
    virtual void getEnabledFeatures()
    {
        if (deviceFeatures.samplerAnisotropy) {
            enabledFeatures.samplerAnisotropy = VK_TRUE;
        }
    };
    void draw()
    {
        vkResetFences(device, 1, &fenceforQueue);

        VulkanExampleBase::prepareFrame();

        std::vector<VkCommandBuffer> cmds;
        cmds.push_back(gPass->commandBuffer);
        cmds.push_back(drawCmdBuffers[currentBuffer]);

        submitInfo.commandBufferCount = cmds.size();
        submitInfo.pCommandBuffers = cmds.data();

        VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

        VulkanExampleBase::submitFrame();
    }

    void buildCommandBuffers() override
    {
        gPass->draw();
        lightPass->draw();
    }


    void prepare() override
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

        // Maximum descriptor sets in pipeline
        uint32_t maxBoundDescriptorSets = deviceProperties.limits.maxBoundDescriptorSets;

        // Maximum descriptors per type in each shader stage
        uint32_t maxUniformBuffersPerStage = deviceProperties.limits.maxPerStageDescriptorUniformBuffers;
        uint32_t maxStorageBuffersPerStage = deviceProperties.limits.maxPerStageDescriptorStorageBuffers;

        // Maximum descriptors across the entire pipeline
        uint32_t maxUniformBuffers = deviceProperties.limits.maxDescriptorSetUniformBuffers;
        uint32_t maxStorageBuffers = deviceProperties.limits.maxDescriptorSetStorageBuffers;

        std::cout << "Max Bound Descriptor Sets: " << maxBoundDescriptorSets << std::endl;
        std::cout << "Max Uniform Buffers per Stage: " << maxUniformBuffersPerStage << std::endl;
        std::cout << "Max Storage Buffers per Stage: " << maxStorageBuffersPerStage << std::endl;
        std::cout << "Max Uniform Buffers in Pipeline: " << maxUniformBuffers << std::endl;
        std::cout << "Max Storage Buffers in Pipeline: " << maxStorageBuffers << std::endl;


        VulkanExampleBase::prepare();
        VkPhysicalDeviceProperties properties;

        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        uint32_t maxSSBOsPerStage = properties.limits.maxPerStageDescriptorStorageBuffers;

        auto fenceCreateInfo = vks::initializers::fenceCreateInfo();
        VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fenceforQueue));

        gPass = new GeometryPass( (this));
        gPass->prepare();

        lightPass = new LightPass(this, gPass);
        lightPass->prepare();

        buildCommandBuffers();
        prepared = true;
    }
    virtual void render()
    {
        if (!prepared)
            return;
        gPass->update();
        draw();
    }
private:
    GeometryPass* gPass = VK_NULL_HANDLE;
    LightPass* lightPass = VK_NULL_HANDLE;
    VkFence fenceforQueue = VK_NULL_HANDLE;
};



VulkanExample *vulkanExample;
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{   if (vulkanExample != NULL)
	{
		vulkanExample->handleMessages(hWnd, uMsg, wParam, lParam);
	}
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{   for (int32_t i = 0; i < __argc; i++)
	{
		VulkanExample::args.push_back(__argv[i]);
	};
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->setupWindow(hInstance, WndProc);
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}