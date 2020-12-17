#version 330

// From vertex shader
in vec2 texcoord;
in vec3 poscoord;

// Application data
uniform sampler2D sampler0;
uniform float time;
uniform vec4 fcolor;
uniform bool distort;

// Output color
layout(location = 0) out  vec4 color;

vec2 transform(vec2 coord)
{
    float y = sin((time*0.8) + 5*coord.y + 5*coord.x) * 0.02;
//	vec2 new_coord = {coord.x, coord.y + y};
//    return new_coord;
    return vec2(coord.x, coord.y + y);
}

void main()
{
    if (distort) {
        vec2 coord = transform(texcoord);
        color = fcolor * texture(sampler0, coord);
    }
    if (distort && poscoord.y < 0.5) {
        color = color + vec4(0, 0, 0.1, 0);
    }
    else {
        color = fcolor * texture(sampler0, vec2(texcoord.x, texcoord.y));
    }
}
