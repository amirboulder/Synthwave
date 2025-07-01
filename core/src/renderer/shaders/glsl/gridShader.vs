#version 460 core
layout (location = 0) in vec3 aPos; // the position variable has attribute position 0
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aTexCoord;

layout(std140, binding = 3) uniform Matrices {
    mat4 view;
    mat4 projection;
};

uniform vec3 gCameraWorldPos;

out vec3 WorldPos;

const vec3 Pos[4] = vec3[4](
    vec3(-100.0, 0.0, -100.0),
    vec3( 100.0, 0.0, -100.0),
    vec3( 100.0, 0.0,  100.0),
    vec3(-100.0, 0.0,  100.0)

    );

const int Indices[6] = int[6](0, 2, 1, 2, 0, 3);

void main()
{
    int Index = Indices[gl_VertexID];

    vec3 vPos3 = Pos[Index];

    vPos3.x += gCameraWorldPos.x;
    vPos3.z += gCameraWorldPos.z;

    vec4 vPos = vec4(vPos3, 1.0);
    gl_Position = projection * view * vPos;

    WorldPos = vPos3;
}