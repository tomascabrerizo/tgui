#version 330

in vec2 vert;
in vec2 uvs;
in vec3 color;

out vec4 fragment;

uniform int res_x;
uniform int res_y;
uniform sampler2D tex;

void main() {
    
    fragment = texture(tex, uvs) * vec4(color, 1.0);
}

