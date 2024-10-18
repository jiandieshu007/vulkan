#version 450

layout (binding = 0) uniform sampler2D samplerposition; // world space
layout (binding = 1) uniform sampler2D samplerNormal;  // world space
layout (binding = 2) uniform sampler2D samplerAlbedo;
layout (binding = 3) uniform samplerCubeArray  pointLightDepthArray;


#define LIGHT_COUNT 1
#define AMBIENT_LIGHT 0.1

struct pointLight{
	vec3 position;
	vec3 intensity;
};

layout (std430, binding = 4) buffer Lights{
	pointLight pointLights[LIGHT_COUNT];
}ubo;

layout (push_constant) uniform PushConstants {
    vec3 viewPos; 
} view;


layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outFragColor;

#define ambient 0.1

float textureProj(vec3 shadowCoord, vec3 off)
{
	float shadow = 0;
	for(int i=0; i<LIGHT_COUNT;++i){
		vec3 lightdir = ubo.pointLights[i].position -shadowCoord;
		float lenDir = length(lightdir);
		float sam = texture(pointLightDepthArray,vec4(normalize(lightdir+off),i)).r;
		shadow += lenDir > sam ? 1 : ambient;
	}
	return shadow;
}

float filterPCF(vec3 sc)
{
	ivec3 texDim = textureSize(pointLightDepthArray, 0);
	float scale = 1.5;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);
	float dz = scale * 1.0 / float(texDim.z);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;
	
	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
			for(int z = -range; z<range; z++){
				shadowFactor += textureProj(sc, vec3(dx*x, dy*y, dz*z));
				count+= LIGHT_COUNT;
			}
		}
	
	}
	return shadowFactor / count;
}


// compute shading in world space
void main() 
{
	vec3 fragPos = texture(samplerposition, inUV).rgb;
	vec3 normal = normalize(texture(samplerNormal, inUV).rgb * 2.0 - 1.0);
	vec4 albedo = texture(samplerAlbedo,inUV).rgba;
	vec3 kd = texture(samplerAlbedo, inUV).rgb;
	vec3 ks = 1-kd;


	// Ambient part
	vec3 fragcolor  = albedo.rgb * AMBIENT_LIGHT;

	vec3 N = normalize(normal);
		
	for(int i = 0; i < LIGHT_COUNT; ++i)
	{
		// Vector to light
		vec3 L = ubo.pointLights[i].position.xyz - fragPos;
		// Distance from light to fragment position
		float dist = length(L);
		L = normalize(L);

		// Viewer to fragment
		vec3 V = view.viewPos.xyz - fragPos;
		V = normalize(V);

		float lightCosInnerAngle = cos(radians(15.0));
		float lightCosOuterAngle = cos(radians(25.0));
		float lightRange = 100.0;

		
		vec3 dir = L;

		// Dual cone spot light with smooth transition between inner and outer angle
		float cosDir = dot(L, dir);


		// Diffuse lighting
		float NdotL = max(0.0, dot(N, L));
		vec3 diff = vec3(NdotL);

		// Specular lighting
		vec3 R = reflect(-L, N);
		float NdotR = max(0.0, dot(R, V));
		vec3 spec = vec3(pow(NdotR, 16.0) * albedo.a * 2.5);

		fragcolor += vec3((diff + spec) ) * ubo.pointLights[i].intensity.rgb * albedo.rgb;
	}    	

	fragcolor *= filterPCF(fragPos);

	// hdr transform
	vec3 outhdrColor = fragcolor/(fragcolor+1);
	
	//gamma 
	outFragColor = vec4(pow(outhdrColor, vec3(1.0 / 2.2)),1);
}