#version 330 

// Input attributes
in vec3 in_position;
in vec2 in_texcoord;

// Passed to fragment shader
out vec2 texcoord;
out vec3 poscoord;

// Application data
uniform mat3 transform;
uniform mat3 projection;

void main()
{
	texcoord = in_texcoord;
	vec3 pos = projection * transform * vec3(in_position.xy, 1.0);
	poscoord = pos;
	gl_Position = vec4(pos.xy, in_position.z, 1.0);
}