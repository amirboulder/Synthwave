#version 460
#extension GL_ARB_bindless_texture : require

in vec2 TexCoord;

out vec4 FragColor;

layout(std430, binding = 4) buffer  textureHandles {
    sampler2D BindlessTextures[];
};

void main() {

    FragColor = texture(BindlessTextures[0], TexCoord);  
}

