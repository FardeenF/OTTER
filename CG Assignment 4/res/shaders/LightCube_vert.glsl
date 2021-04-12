#version 420 core
layout(location = 0) in vec3 inPos;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_ModelViewProjection;

void main()
{
    gl_Position = u_ModelViewProjection * u_View * u_Model * vec4(inPos, 1.0);
}