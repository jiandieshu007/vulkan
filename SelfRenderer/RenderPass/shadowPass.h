#include"RenderPass.h"
#include "Geometry.h"




const float nearPlane = 0.1f;
const float farPlane = 25.0f;
const float fov = 90.0f;


// ���� lookAt ����� up ������������
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
    PointLight Light;
    glm::mat4 view[6];
    vks::Framebuffer framebuffer[6];
};

class ShadowPass : public RenderPass {
public:
    ShadowPass(VulkanExampleBase* example, GeometryPass* pass) : RenderPass(example), gPass(pass) {}

    virtual void prepare() override;
    virtual void draw() override;
    virtual void update() override;
    virtual void createRenderpass() override;
    std::array<pointLightsWithFramebuffer, LightCount> pointLights{};

    void initLights();
    vks::Framebuffer initShadowFramebuffer();
    GeometryPass* gPass;
    vks::Buffer lightBuffer;
    VkCommandBuffer commandBuffer;
};



