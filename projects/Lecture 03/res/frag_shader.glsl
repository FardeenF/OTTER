#version 410

layout(location = 1) in vec3 inColor;

out vec4 frag_color;

uniform vec3 color = vec3(1.0, 0.0, 0.0);

void main(){

	frag_color = vec4(inColor * color, 1.0);
}