#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aTexCoord;


layout(std140, binding = 3) uniform Matrices {
    mat4 view;
    mat4 projection;
};

uniform mat4 model;

out vec3 vertex;
out vec3 cameraPos;

void main()
{

    // The camera position is the negative of the translation part of the view matrix
    // after inverting the rotation part
    mat3 rotMat = mat3(view); // Extract the 3x3 rotation part
    vec3 translationPart = vec3(view[3]); // Extract the translation part (fourth column)

    // Calculate camera position in world space
    cameraPos = -transpose(rotMat) * translationPart;

    vertex = aPos.xyz;

    gl_Position = projection * view * model * vec4(aPos, 1.0);

}