#version 410
layout(location = 0) in vec2 inUV;

uniform sampler2D s_Texture;

out vec4 frag_color;

void main() {
	vec4 result = texture(s_Texture, inUV);

	frag_color = result;
}