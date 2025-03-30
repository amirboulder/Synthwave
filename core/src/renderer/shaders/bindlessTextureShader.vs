#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aTexCoord;

layout(std140, binding = 3) uniform Matrices {
    mat4 view;
    mat4 projection;
};

uniform mat4 model;
out vec2 TexCoord;

void main()
{
    TexCoord = aTexCoord;

    gl_Position = projection * view * model * vec4(aPos, 1.0);

}