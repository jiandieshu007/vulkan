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
        submitInfo.pWaitSemaphores = &semaphores.presentComplete;
        // Signal ready with offscreen semaphore
        submitInfo.pSignalSemaphores = &gPass->geometey;

        // Submit work
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &gPass->commandBuffer;
        VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

        // Scene rendering

        // Wait for offscreen semaphore
        submitInfo.pWaitSemaphores = &gPass->geometey;
        // Signal ready with render complete semaphore
        submitInfo.pSignalSemaphores = &semaphores.renderComplete;

        // Submit work
        submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];

        VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fenceforQueue));

        vkWaitForFences(device, 1, &fenceforQueue, VK_TRUE, UINT64_MAX);



        VulkanExampleBase::submitFrame();
    }

    void buildCommandBuffers() override
    {
        gPass->draw();
        lightPass->draw();
    }


    void prepare() override
    {
        VulkanExampleBase::prepare();

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
    system("pause");
	return 0;
}