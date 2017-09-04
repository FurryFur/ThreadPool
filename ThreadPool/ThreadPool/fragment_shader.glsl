#version 450 core

in vec2 TexCoord;

out vec4 Color;

uniform sampler2D uSampler;

void main(void)
{
    Color = texture(uSampler, TexCoord);
}
