#version 410

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;

layout(location = 0) out vec3 outPos;
layout(location = 1) out vec3 outColor;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec2 outUV;

uniform mat4 u_ModelViewProjection;
uniform mat4 u_View;
uniform mat4 u_Model;
uniform mat3 u_NormalMatrix;
uniform vec3 u_LightPos;

uniform sampler2D s_Water;

uniform float time;


void main() {
	
	outPos = (u_Model * vec4(inPosition, 1.0)).xyz;

	outNormal = u_NormalMatrix * inNormal;

	outColor = inColor;

	outUV = inUV * 64;

	vec3 vert = inPosition;

	vert.z = sin(vert.x * 16.0 + time * 0.5) * 0.01;


	gl_Position = u_ModelViewProjection * vec4(vert, 1.0);

}
	