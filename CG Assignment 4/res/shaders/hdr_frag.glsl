#version 420

out vec4 frag_color;
  
in vec4 gl_FragCoord;

layout (binding = 4) uniform sampler2D s_lightAccumTex;

void main()
{   
	const float gamma = 2.2;
	vec2 screenUV = vec2(gl_FragCoord.x / 800.0, gl_FragCoord.y / 800.0);
    vec3 hdrColor = texture(s_lightAccumTex, screenUV).rgb;

	 // reinhard tone mapping
    vec3 mapped = hdrColor / (hdrColor + vec3(1.0));
    // gamma correction 
    mapped = pow(mapped, vec3(1.0 / gamma));

    frag_color = vec4(mapped, 1.0);
}  