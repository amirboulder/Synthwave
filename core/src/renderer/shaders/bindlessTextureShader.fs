#version 460
#extension GL_ARB_bindless_texture : require

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D BindlessTexture;

void main() {
    FragColor = texture(BindlessTexture, TexCoord);
}