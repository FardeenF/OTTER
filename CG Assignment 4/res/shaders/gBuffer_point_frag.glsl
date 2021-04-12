#version 420

layout(location = 0) in vec2 inUV;

struct DirectionalLight
{
	//Light direction (defaults to down, to the left, and a little forward)
	vec4 _lightDirection;

	//Generic Light controls
	vec4 _lightCol;

	//Ambience controls
	vec4 _ambientCol;
	float _ambientPow;
	
	//Power controls
	float _lightAmbientPow;
	float _lightSpecularPow;
	
	float _shadowBias;
};

struct PointLight
{
	//Light Position
	vec4 _lightPos;

	//Generic Light controls
	vec4 _lightCol;

	//Ambience controls
	vec4 _ambientCol;
	
	float _lightLinearFalloff;
	float _lightQuadraticFalloff;
	float _ambientPow;
	float _lightAmbientPow;
	float _lightSpecularPow;
};


layout (std140, binding = 0) uniform u_Lights
{
	PointLight sun;
};

layout (binding = 30) uniform sampler2D s_ShadowMap;

layout (binding = 0) uniform sampler2D s_albedoTex;
layout (binding = 1) uniform sampler2D s_normalsTex;
layout (binding = 2) uniform sampler2D s_specularTex;
layout (binding = 3) uniform sampler2D s_positionTex;

layout (binding = 4) uniform sampler2D s_lightAccumTex;

uniform mat4 u_LightSpaceMatrix;
uniform vec3 u_CamPos;

out vec4 frag_color;



// calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light._lightPos - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // attenuation
    float distance = length(light._lightPos - fragPos);
    float attenuation = 1.0 / (2.0 + light.linear * distance + 4.0 * (distance * distance));
    // combine results
    vec3 ambient = light._ambientPow * vec3(texture(material.diffuse, inUV));
    vec3 diffuse = light._ * diff * vec3(texture(material.diffuse, inUV));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, inUV));
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}



// https://learnopengl.com/Advanced-Lighting/Advanced-Lighting
void main() 
{
	//Albedo
	vec4 textureColor = texture(s_albedoTex, inUV);
	//Normals
	vec3 inNormal = (normalize(texture(s_normalsTex, inUV).rgb) * 2.0) - 1.0;
	//Specular
	float texSpec = texture(s_specularTex, inUV).r;
	//Positions
	vec3 fragPos = texture(s_positionTex, inUV).rgb;
	
//	// Diffuse
//	vec3 N = normalize(inNormal);
//	vec3 lightDir = normalize(-sun._lightDirection.xyz);
//	float dif = max(dot(N, lightDir), 0.0);
//	vec3 diffuse = dif * sun._lightCol.xyz;// add diffuse intensity

	// Specular
	vec3 viewDir  = normalize(u_CamPos - fragPos);
//	vec3 h        = normalize(lightDir + viewDir);

	float spec = pow(max(dot(N, h), 0.0), 4.0); // Shininess coefficient (can be a uniform)
	vec3 specular = sun._lightSpecularPow * texSpec * spec * sun._lightCol.xyz; // Can also use a specular color

//	vec4 fragPosLightSpace = u_LightSpaceMatrix * vec4(fragPos, 1.0);
//	float shadow = ShadowCalculation(fragPosLightSpace, sun._shadowBias);

//	vec3 result = (
//		(sun._ambientPow * sun._ambientCol.xyz) + // global ambient light
//		(1.0 - shadow) * //Shadow value
//		(diffuse + specular)); // Object color

	vec3 result = CalcPointLight(sun, inNormal, fragPos, viewDir);

	if(textureColor.a < 0.31)
	{
		result = vec3(1.0, 1.0, 1.0);
	}

	frag_color = vec4(result, 1.0);
}
