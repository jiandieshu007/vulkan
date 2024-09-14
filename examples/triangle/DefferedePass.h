
#pragma

#include "RenderPass.h"

class GeometryPass : public RenderPass {
public:
	GeometryPass(vks::VulkanDevice& device) : RenderPass(device){}

	virtual void prepare();

	virtual void draw();
};