#version 330

layout (location = 0) in vec2 aVert;
layout (location = 1) in vec2 aUvs;
layout (location = 2) in vec3 aColor;

uniform int res_x;
uniform int res_y;

out vec2 vert;
out vec2 uvs;
out vec3 color;

void main() {
   
    vec2 screen = vec2(res_x, res_y);
    vert.x = (aVert.x / screen.x) *  2 - 1;
    vert.y = (aVert.y / screen.y) * -2 + 1;

    uvs   = aUvs;
    color = aColor;

    gl_Position = vec4(vert, 0, 1);
}

