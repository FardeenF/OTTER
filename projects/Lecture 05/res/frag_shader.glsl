#version 410

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

uniform vec3 LightPos;


out vec4 frag_color;


void main() {
	
	//Lecture 5
	vec3 lightColor = vec3(1.0, 1.0, 1.0);

	float ambientStrength = 0.1;
	vec3 ambient = ambientStrength * lightColor * inColor;

	//Diffuse
	vec3 N = normalize(inNormal);
	vec3 lightDir = normalize(LightPos - inPos);

	float dif = max(dot(N, lightDir), 0.0); //If negative turns it to 0
	vec3 diffuse = dif * inColor; //Can add diffuse intensity => * 4.0

	//Attenuation
	float dist = length(LightPos - inPos);
	diffuse = diffuse / dist; // (dist*dist)

	//Specular
	vec3 camPos = vec3(0.0, 0.0, 3.0); //Pass this as a uniform from your C++ code
	float specularStrength = 1.0; //This can be a uniform
	vec3 camDir = normalize(camPos - inPos);
	vec3 reflectDir = reflect(-lightDir, N);
	float spec = pow(max(dot(camDir, reflectDir), 0.0), 256); //Shininess coefficient
	vec3 specular = specularStrength * spec * lightColor; //Can also use specular color


	vec3 result = (ambient + diffuse + specular);

	frag_color = vec4(result, 1.0);
}