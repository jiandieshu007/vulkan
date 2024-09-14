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

    }

    void draw()
    {
        VulkanExampleBase::prepareFrame();


        VulkanExampleBase::submitFrame();
    }

    void prepare() override
    {
        VulkanExampleBase::prepare();

        prepared = true;
    }
    virtual void render()
    {
        if (!prepared)
            return;
        draw();
    }
};



VULKAN_EXAMPLE_MAIN()