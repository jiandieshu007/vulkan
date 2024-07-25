#version 440 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUv;

layout (binding = 1) uniform sampler2D samplerColor;

layout(location = 0) out vec4 outColor;

void main(){
	outColor = texture(samplerColor,inUv);
}