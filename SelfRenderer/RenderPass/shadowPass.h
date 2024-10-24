#pragma once

#include"RenderPass.h"
#include "Geometry.h"




const float nearPlane = 0.1f;
const float farPlane = 25.0f;
const float fov = 90.0f;


// 定义 lookAt 方向和 up 向量常量数组
const glm::vec3 directions[6] = {
    glm::vec3(1.0f,  0.0f,  0.0f),  // +X
    glm::vec3(-1.0f, 0.0f,  0.0f),  // -X
    glm::vec3(0.0f,  1.0f,  0.0f),  // +Y
    glm::vec3(0.0f, -1.0f,  0.0f),  // -Y
    glm::vec3(0.0f,  0.0f,  1.0f),  // +Z
    glm::vec3(0.0f,  0.0f, -1.0f)   // -Z
};

const glm::vec3 upVectors[6] = {
    glm::vec3(0.0f, -1.0f,  0.0f),  // +X
    glm::vec3(0.0f, -1.0f,  0.0f),  // -X
    glm::vec3(0.0f,  0.0f,  1.0f),  // +Y
    glm::vec3(0.0f,  0.0f, -1.0f),  // -Y
    glm::vec3(0.0f, -1.0f,  0.0f),  // +Z
    glm::vec3(0.0f, -1.0f,  0.0f)   // -Z
};


struct pointLightsWithFramebuffer
{
    ~pointLightsWithFramebuffer()
    {
	    for(int i=0; i<6;++i)
	    {
            framebuffer[i]->~Framebuffer();
	    }
    }
    PointLight Light;
    glm::mat4 model;
    glm::mat4 view[6];
    vks::Framebuffer* framebuffer[6] ;
};

class ShadowPass : public RenderPass {
public:
    ShadowPass(VulkanExampleBase* example, GeometryPass* pass) : RenderPass(example), gPass(pass) {}
    ~ShadowPass()
    { 
        lightBuffer.destroy();
    }
    virtual void prepare() override;
    virtual void draw() override;
    virtual void update(VkQueue& queue) override;
    virtual void createRenderpass() override;

    std::vector<pointLightsWithFramebuffer> pointLights;
    const int depthsWidth = 1024;
    const int depthsHeight = 1024;
    void initLights();
    void ShadowPass::initShadowFramebuffer(vks::Framebuffer* framebuffer);

    GeometryPass* gPass;
    vks::Buffer lightBuffer;
    VkCommandBuffer commandBuffer;
};



