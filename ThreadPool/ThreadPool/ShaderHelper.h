//
// Bachelor of Software Engineering
// Media Design School
// Auckland
// New Zealand
//
// (c) 2017 Media Design School
//
// Description  : Utils for shader operations
// Author       : Lance Chaney
// Mail         : lance.cha7337@mediadesign.school.nz
//

#ifndef _SHADER_HELPER__
#define _SHADER_HELPER__

#include <string>
#include <glad\glad.h>

//std::string readShaderFileFromResource(const char* pFileName);
//GLuint compileVertexShader(const char* shaderCode);
//GLuint compileFragmentShader(const char* shaderCode);
//GLuint compileShader(GLenum ShaderType, const char* shaderCode);
//GLuint linkProgram(GLuint vertexShaderId, GLuint fragmentShaderId);
//GLint validateProgram(GLuint programObjectId);

// Compile and link the shader programs.
// vertex_shader is the file path to the vertex_shader code.
// fragment_shader is the file path to the fragment_shader code.
// program is returned by reference into last parameter.
void compileAndLinkShaders(std::string vertex_shader, std::string fragment_shader, GLuint& program);

#endif