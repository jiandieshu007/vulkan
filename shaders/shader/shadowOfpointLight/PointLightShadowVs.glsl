#version 450

layout( location = 0) in vec4 inPos;

layout (push_constant) uniform PushConstants {
    mat4 view; 
}Light;

void main(){
    gl_Position = Light.view * inPos;
}

