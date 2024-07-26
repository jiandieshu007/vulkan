#version 450

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inNormal;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 modelview;
} ubo;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec3 outViewPos;
layout (location = 3) out vec3 outTangent;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main() 
{
	gl_Position = ubo.projection * ubo.modelview * inPos;
	
	// Vertex position in world space
	outViewPos = vec3(ubo.modelview * inPos);
	// GL to Vulkan coord space
	outViewPos.y = -outViewPos.y;
	
	// Normal in world space
	mat3 mNormal = transpose(inverse(mat3(ubo.modelview)));
	outNormal = mNormal * normalize(inNormal);	
	
	// Currently just vertex color
	outColor = inColor;
}
