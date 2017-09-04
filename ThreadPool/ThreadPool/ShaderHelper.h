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

void compileAndLinkShaders(std::string vertex_shader, std::string fragment_shader, GLuint& program);

#endif