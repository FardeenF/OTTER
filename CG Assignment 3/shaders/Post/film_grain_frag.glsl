#version 420

layout(location = 0) in vec2 inUV;

out vec4 frag_color;

layout (binding = 0) uniform sampler2D s_screenTex;

//Affects grainy the effect is
//Lower the number, closer we are to regular
uniform float u_Intensity = 0.1;

void main() 
{
	vec4 source = texture(s_screenTex, inUV);

	float noiseIntensity = u_Intensity;
	float noise = fract(sin(dot(inUV, vec2(12.9898,78.233)))*43578.5453);

	vec4 finalSource = source - noise * noiseIntensity;

	frag_color = finalSource;

}