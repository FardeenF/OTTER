#version 420 core
layout(location = 0) in vec3 inPos; //was aPos 
layout(location = 1) in vec3 inNormal; //was aNormal
layout(location = 2) in vec2 inUVs; //was aTexCoords

layout(location = 0) out vec3 outPos; // was FragPos
layout(location = 1) out vec3 outNormal; // was Normal
layout(location = 2) out vec2 outUV; // was TexCoords

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_ModelViewProjection;

void main()
{
    outPos = vec3(u_Model * vec4(inPos, 1.0));
    outNormal = mat3(transpose(inverse(u_Model))) * inNormal;
    outUV = inUVs;

    gl_Position = u_ModelViewProjection * u_View * vec4(outPos, 1.0);
}