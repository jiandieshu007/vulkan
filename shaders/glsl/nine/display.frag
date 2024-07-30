#version 450

#extension GL_KHR_vulkan_glsl : enable

layout (input_attachment_index = 0,set = 0, binding = 0) uniform subpassInput inputPosition;
layout (input_attachment_index = 1,set = 0, binding = 1) uniform subpassInput inputNormal;
layout (input_attachment_index = 2, set = 0,  binding = 2) uniform subpassInput inputAlbedo;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;



void main() 
{
	// Read G-Buffer values from previous sub pass
	vec3 fragPos = subpassLoad(inputPosition).rgb;
	vec3 normal = subpassLoad(inputNormal).rgb;
	vec4 albedo = subpassLoad(inputAlbedo);
	const int index = 1;

	if( index == 0){
		outColor = vec4(fragPos,1);
	}else if(index == 1){
		outColor = vec4(normal,1);
	}else{
		outColor = albedo;
	}
}