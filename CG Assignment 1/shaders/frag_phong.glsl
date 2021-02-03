#version 410

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;

uniform bool u_LightToggle;
uniform bool u_AmbientToggle;
uniform bool u_SpecularToggle;
uniform bool u_AmbientSpecularToggle;
uniform bool u_CustomToggle;

uniform sampler2D s_Diffuse;
uniform sampler2D s_Diffuse2;

uniform float u_TextureMix;

uniform vec3  u_AmbientCol;
uniform float u_AmbientStrength;
uniform float u_SpecularLightStrength;

uniform vec3  u_LightPos;
uniform vec3  u_LightCol;
uniform float u_AmbientLightStrength;
uniform float u_Shininess;

uniform vec3  u_CamPos;

out vec4 frag_color;


const int bands = 3;
const float scaleFactor = 1.0/bands;
uniform float u_OutlineThickness;

void main() {
	// Ambient
	vec3 ambient = ((u_AmbientLightStrength * u_LightCol) + (u_AmbientCol * u_AmbientStrength));

	// Diffuse
	vec3 N = normalize(inNormal);
	vec3 lightDir = normalize(u_LightPos - inPos);

	float dif = max(dot(N, lightDir), 0.0);
	vec3 diffuse = dif * u_LightCol;// add diffuse intensity

	//Attenuation
	float dist = length(u_LightPos - inPos);
	diffuse = diffuse / dist; // (dist*dist)

	float attenuation = 1.0f / (
		1.0 + 
		0.09 * dist +
		0.032 * dist * dist);


	// Specular
	vec3 camPos = u_CamPos;//Pass this as a uniform from your C++ code
	float specularStrength = u_SpecularLightStrength; // this can be a uniform
	vec3 camDir = normalize(camPos - inPos);
	vec3 reflectDir = reflect(-lightDir, N);
	float spec = pow(max(dot(camDir, reflectDir), 0.0), u_Shininess); // Shininess coefficient (can be a uniform)
	vec3 specular = specularStrength * spec * u_LightCol; // Can also use a specular color

	//Outline Effect             Thickness of Line
	float edge = (dot(camDir, N) < u_OutlineThickness) ? 0.0 : 1.0; //If below threshold it is 0, otherwise 1

	vec3 lightContribution = (ambient + diffuse + specular) * attenuation;

	lightContribution = clamp(floor(lightContribution * bands) * scaleFactor, vec3(0.0), vec3(1.0));

	// Get the albedo from the diffuse / albedo map
	vec4 textureColor1 = texture(s_Diffuse, inUV);
	vec4 textureColor2 = texture(s_Diffuse2, inUV);
	vec4 textureColor = mix(textureColor1, textureColor2, u_TextureMix);

	vec3 result;

	if(u_LightToggle == true)
	{
		result = inColor * textureColor.rgb;
	}

	if(u_AmbientToggle == true)
	{
		result = (ambient) * inColor * textureColor.rgb;
	}

	if(u_SpecularToggle == true)
	{
		result = (specular) * inColor * textureColor.rgb;
	}

	if(u_AmbientSpecularToggle == true)
	{
		result = (ambient + specular) * inColor * textureColor.rgb;
	}

	if(u_CustomToggle == true)
	{
		result = (ambient + specular) * inColor * textureColor.rgb;
	}


	frag_color = vec4(result, 1.0);
}