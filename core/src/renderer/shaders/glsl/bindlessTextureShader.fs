#version 460
#extension GL_ARB_bindless_texture : require

in vec2 TexCoord;
out vec4 FragColor;

layout(std430, binding = 4) buffer textureHandles {
    uvec2 BindlessTextures[]; 
};

void main() {
    FragColor = texture(sampler2D(BindlessTextures[0]), TexCoord);
}