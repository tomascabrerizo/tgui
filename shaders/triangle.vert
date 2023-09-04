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
    
    vert  = aVert;
    uvs   = aUvs;
    color = aColor;

    gl_Position = vec4(vert, 0, 1);
} 
