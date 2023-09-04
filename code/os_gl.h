#ifndef _OS_GL_H_
#define _OS_GL_H_

/* TODO: Find a good way to not have duplicated symbols */

#define glDrawArrays glDrawArraysOld
#define glGetTexImage glGetTexGendvOld
#define glTexSubImage2D glTexSubImage2DOld
#define glDrawPixels glDrawPixelsOld
#define glReadPixels glReadPixelsOld
#define glDrawElements glDrawElementsOld
#define glGenTextures glGenTexturesOld
#define glBindTexture glBindTextureOld
#define glTexParameterf glTexParameterfOld
#define glTexParameteri glTexParameteriOld
#define glDeleteTextures glDeleteTexturesOld
#define glTexImage2D glTexImage2DOld 
#include <GL/gl.h>
#undef glGetTexImage
#undef glTexSubImage2D
#undef glDrawArrays
#undef glDrawPixels
#undef glReadPixels
#undef glDrawElements
#undef glGenTextures
#undef glBindTexture
#undef glTexParameterf
#undef glTexParameteri
#undef glDeleteTextures
#undef glTexImage2D

#include <GL/glx.h>

/* TODO: Add GL types */

#define GL_FUNCTIONS(X) \
  X(void, glGenVertexArrays, (GLsizei n, GLuint *arrays)) \
  X(void, glBindVertexArray, (GLuint array)) \
  X(void, glGenBuffers, (GLsizei	n, GLuint *buffers)) \
  X(void, glBindBuffer, (GLenum target, GLuint buffer)) \
  X(void, glBufferData, (GLenum	target, GLsizeiptr	size, const GLvoid *data, GLenum	usage)) \
  X(void, glGenFramebuffers, (GLsizei n, GLuint *ids)) \
  X(void, glBindFramebuffer, (GLenum target, GLuint framebuffer)) \
  X(void, glFramebufferTexture2D, (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)) \
  X(void, glGenRenderbuffers, (GLsizei n, GLuint *renderbuffers)) \
  X(void, glBindRenderbuffer, (GLenum target, GLuint renderbuffer)) \
  X(void, glRenderbufferStorage, (GLenum target, GLenum internalformat, GLsizei width, GLsizei height)) \
  X(void, glFramebufferRenderbuffer, (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)) \
  X(GLenum, glCheckFramebufferStatus, (GLenum target)) \
  X(void, glDeleteRenderbuffers, (GLsizei n, GLuint *renderbuffers)) \
  X(void, glDeleteFramebuffers, (GLsizei n, GLuint *framebuffers)) \
  X(GLuint, glCreateShader, (GLenum shaderType)) \
  X(void, glShaderSource, (GLuint shader, GLsizei count, const GLchar **string, const GLint *length)) \
  X(void, glGetShaderSource, (GLuint	shader, GLsizei	bufSize, GLsizei *length, GLchar *source)) \
  X(void, glCompileShader, (GLuint shader)) \
  X(void, glGetShaderiv, (GLuint shader, GLenum pname, GLint *params)) \
  X(void, glGetShaderInfoLog, (GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog)) \
  X(GLuint, glCreateProgram, (void)) \
  X(void, glAttachShader, (GLuint program, GLuint shader)) \
  X(void, glLinkProgram, (GLuint program)) \
  X(void, glGetProgramiv, (GLuint program, GLenum pname, GLint *params)) \
  X(void, glGetProgramInfoLog, (GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog)) \
  X(void, glDeleteShader, (GLuint shader)) \
  X(void, glDeleteProgram, (GLuint program)) \
  X(void, glUseProgram, (GLuint program)) \
  X(void, glVertexAttribPointer, (GLuint	index, GLint	size, GLenum	type, GLboolean	normalized, GLsizei	stride, const GLvoid *pointer)) \
  X(void, glVertexAttribIPointer, (GLuint	index, GLint	size, GLenum	type, GLsizei	stride, const GLvoid *pointer)) \
  X(void, glEnableVertexAttribArray, (GLuint	index)) \
  X(void, glDisableVertexAttribArray, (GLuint	index)) \
  X(void, glDrawArrays, (GLenum	mode, GLint	first, GLsizei	count)) \
  X(void, glDeleteBuffers, (GLsizei	n, const GLuint *	buffers)) \
  X(void, glDeleteVertexArrays, (GLsizei n, const GLuint *arrays)) \
  X(void, glGetTexImage, (GLenum	target, GLint	level, GLenum	format, GLenum type, GLvoid *	img)) \
  X(void, glTexSubImage2D, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *data)) \
  X(void, glDrawPixels, (GLsizei	width, GLsizei	height, GLenum	format, GLenum	type, const GLvoid *data)) \
  X(void, glReadPixels, (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *data)) \
  X(void, glUniformMatrix4fv, (GLint location, GLsizei	count, GLboolean	transpose, const GLfloat *	value)) \
  X(GLint, glGetUniformLocation, (GLuint program, const GLchar *name)) \
  X(void, glDrawElements, (GLenum	mode, GLsizei	count, GLenum	type, const GLvoid *indices)) \
  X(void, glDrawArraysInstanced, (GLenum mode, GLint first, GLsizei count, GLsizei primcount)) \
  X(void, glVertexAttribDivisor, (GLuint index, GLuint divisor)) \
  X(void, glUniform2f, (GLint	location, GLfloat	v0, GLfloat	v1)) \
  X(void, glUniform1i, (GLint location, GLint v0)) \
  X(void, glBufferSubData, (GLenum	target, GLintptr	offset, GLsizeiptr size, const GLvoid *data)) \
  X(void, glGenTextures, (GLsizei	n, GLuint *textures)) \
  X(void, glBindTexture, (GLenum	target, GLuint	texture)) \
  X(void, glTexParameterf, (GLenum target, GLenum	pname, GLfloat	param)) \
  X(void, glTexParameteri, (GLenum target, GLenum	pname, GLint	param)) \
  X(void, glDeleteTextures, (GLsizei	n, const GLuint *textures)) \
  X(void, glTexImage2D, (GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * data)) \
  X(void, glBlitFramebuffer, (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter))

#define TGUI_GL_PROC(name) TGUI_##name##_PROC

#define X(return, name, params) typedef return (*TGUI_GL_PROC(name)) params;
GL_FUNCTIONS(X)
#undef X

#define X(return, name, params) extern TGUI_GL_PROC(name) name;
GL_FUNCTIONS(X)
#undef X


#endif /* _OS_GL_H_ */
