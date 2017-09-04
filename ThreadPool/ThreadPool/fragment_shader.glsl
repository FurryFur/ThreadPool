#version 450 core

in vec2 TexCoord;

out vec4 Color;

uniform sampler2D texSampler;

void main(void)
{
    Color = texture(texSampler, TexCoord);
}
