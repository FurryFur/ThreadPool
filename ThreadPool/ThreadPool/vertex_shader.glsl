#version 450 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 uScale;
uniform mat4 uRotate;
uniform mat4 uTranslate;
uniform mat4 uView;
uniform mat4 uProjection;

out vec2 TexCoord;

void main() 
{
    gl_Position = uProjection * uView * uTranslate * uRotate * uScale * vec4(aPosition, 1.0f);
    TexCoord = aTexCoord;
}