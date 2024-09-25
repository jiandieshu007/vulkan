#version 450

layout (binding = 0) uniform sampler2D samplerposition;
layout (binding = 1) uniform sampler2D samplerNormal;
layout (binding = 2) uniform sampler2D samplerAlbedo;
layout (binding = 3) uniform sampler2D samplerDepth;


#define LightCount 32

layout (std430, binding = 4) buffer  PointLight{
	vec3 position;
	vec3 intensity;
} Lights[LightCount];

layout (push_constant) uniform PushConstants {
    vec3 viewPos; 
} view;


layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outFragColor;

vec3 ao = vec3(0);

void main() 
{
	vec3 fragPos = texture(samplerposition, inUV).rgb;
	vec3 normal = normalize(texture(samplerNormal, inUV).rgb * 2.0 - 1.0);
	vec3 kd = texture(samplerAlbedo, inUV).rgb;
	vec3 ks = 1-kd;
	vec3 outColor = ao;

	for (int i = 0; i < LightCount; ++i) {
        vec3 l = Lights[i].position - fragPos;
		float r = l.length();

		l = normalize(l);
		vec3 v = normalize(view.viewPos-fragPos);

		vec3 h = normalize(-l+v);
		vec3 id = kd * Lights[i].intensity * max(0, dot(normal,-l))/r/r;
		vec3 is = ks * Lights[i].intensity * pow( max(0, dot(normal,-l)),8) /r/r;
		outColor += id + is;



    }

		// hdr transform
	vec3 outhdrColor = outColor/(outColor+1);
	
	//gamma 
	outFragColor = vec4(pow(outhdrColor, vec3(1.0 / 2.2)),1);
}