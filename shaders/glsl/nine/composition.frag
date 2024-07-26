#version 450

layout (input_attachment_index = 0, binding = 0) uniform subpassInput inputPosition;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput inputNormal;
layout (input_attachment_index = 2, binding = 2) uniform subpassInput inputAlbedo;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;



layout(push_constant) uniform pushConstant{
	uint32_t index;
}Constant;

void main() 
{
	// Read G-Buffer values from previous sub pass
	vec3 fragPos = subpassLoad(inputPosition).rgb;
	vec3 normal = subpassLoad(inputNormal).rgb;
	vec4 albedo = subpassLoad(inputAlbedo);
	
	if( index == 0){
		outColor = vec4(fragPos,1);
	}else if(index == 1){
		outColor = vec4(normal,1);
	}else{
		outColor = albedo;
	}
}