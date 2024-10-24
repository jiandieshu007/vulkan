#version 450

layout (binding = 0) uniform sampler2D color;


layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outFragColor;

float computeLuma(vec3 color) {
    return dot(color, vec3(0.299, 0.587, 0.114));
}

void main(){
	
}