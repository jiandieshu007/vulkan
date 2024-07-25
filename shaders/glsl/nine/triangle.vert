#version 440 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 uv;

layout(binding = 0) uniform UBO{
	mat4 Pro;
	mat4 modelView;
}ubo;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUv;

void main(){
	outPosition = (ubo.modelView * vec4(inPosition,1.0)).xyz;
	gl_Position = ubo.Pro * ubo.modelView * vec4(inPosition,1.0);
	outNormal = (inverse(ubo.modelView)* vec4(inNormal,1)).xyz;
	outUv = uv;


}