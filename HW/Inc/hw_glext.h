#ifdef WIN32 /* [ */

#define GL_VERSION_3_0 1        /* We're defining a subset of GL 3.0 func. */

/***************************************************************************
 * Vertex buffer object
 ***************************************************************************/
#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_STATIC_DRAW                    0x88E4
#define GL_DYNAMIC_DRAW                   0x88E8

typedef void (APIENTRY
    *GL_GEN_BUFFERS_FPTR)( GLsizei n, GLuint *buffers );
typedef void (APIENTRY
    *GL_BIND_BUFFER_FPTR)( GLenum target, GLuint buffer );
typedef void (APIENTRY
    *GL_BUFFER_DATA_FPTR)( GLenum target, GLsizei size, const GLvoid *data, GLenum usage );
typedef void (APIENTRY
    *GL_BUFFER_SUB_DATA_FPTR)( GLenum target, GLint offset, GLsizei size, const GLvoid *data );
typedef void (APIENTRY
    *GL_DELETE_BUFFERS_FPTR)( GLsizei n, const GLuint *buffers );

extern GL_GEN_BUFFERS_FPTR      glGenBuffers;
extern GL_BIND_BUFFER_FPTR      glBindBuffer;
extern GL_BUFFER_DATA_FPTR      glBufferData;
extern GL_BUFFER_SUB_DATA_FPTR  glBufferSubData;
extern GL_DELETE_BUFFERS_FPTR   glDeleteBuffers;

/***************************************************************************
 * Multitexture and other misc texture
 ***************************************************************************/
typedef void
    (APIENTRY *GL_ACTIVE_TEXTURE_FPTR)( GLenum tid );
typedef void
    (APIENTRY *GL_CLIENT_ACTIVE_TEXTURE_FPTR)( GLenum tid );

extern GL_ACTIVE_TEXTURE_FPTR               glActiveTexture;
extern GL_CLIENT_ACTIVE_TEXTURE_FPTR        glClientActiveTexture;

#define GL_MAX_TEXTURE_UNITS              0x84E2
#define GL_UNSIGNED_SHORT_4_4_4_4         0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1         0x8034
#define GL_UNSIGNED_SHORT_5_6_5           0x8363
#define GL_CLAMP_TO_EDGE                  0x812F

/***************************************************************************
 * Shader objects
 ***************************************************************************/

#define GL_COMPILE_STATUS                 0x8B81
#define GL_INFO_LOG_LENGTH                0x8B84
#define GL_SHADER_SOURCE_LENGTH           0x8B88
#define GL_LINK_STATUS                    0x8B82
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31

typedef char GLchar;
typedef unsigned int GLhandle;

typedef void (APIENTRY
    *GL_GET_SHADER_IV_FPTR)( GLhandle obj, GLenum pname, GLint *param );
typedef void (APIENTRY
    *GL_GET_PROGRAM_IV_FPTR)( GLhandle obj, GLenum pname, GLint *param );
typedef void (APIENTRY
    *GL_GET_SHADER_SOURCE_FPTR)( GLhandle obj, GLsizei maxLength,
                                 GLsizei *length, GLchar *source);
typedef void (APIENTRY
    *GL_GET_SHADER_INFO_LOG_FPTR)( GLhandle obj, GLsizei maxLength,
                                   GLsizei *length, GLchar *source);
typedef void (APIENTRY
    *GL_GET_PROGRAM_INFO_LOG_FPTR)( GLhandle obj, GLsizei maxLength,
                                    GLsizei *length, GLchar *source);
typedef GLint (APIENTRY
    *GL_GET_UNIFORM_LOCATION_FPTR)( GLhandle program, const GLchar *name );
typedef void (APIENTRY
    *GL_UNIFORM_1FV_FPTR)( GLint loc, GLsizei count, const GLfloat *value );
typedef void (APIENTRY
    *GL_UNIFORM_2FV_FPTR)( GLint loc, GLsizei count, const GLfloat *value );
typedef void (APIENTRY
    *GL_UNIFORM_3FV_FPTR)( GLint loc, GLsizei count, const GLfloat *value );
typedef void (APIENTRY
    *GL_UNIFORM_4FV_FPTR)( GLint loc, GLsizei count, const GLfloat *value );
typedef void (APIENTRY
    *GL_UNIFORM_1I_FPTR)( GLint loc, GLint v0 );
typedef void (APIENTRY
    *GL_UNIFORM_MATRIX_3FV_FPTR)( GLint loc, GLsizei count,
                                  GLboolean transpose, const GLfloat *value);
typedef void (APIENTRY
    *GL_UNIFORM_MATRIX_4FV_FPTR)( GLint loc, GLsizei count,
                                  GLboolean transpose, const GLfloat *value);

typedef GLhandle (APIENTRY
    *GL_CREATE_SHADER_FPTR)( GLenum pname );
typedef void (APIENTRY
    *GL_SHADER_SOURCE_FPTR)( GLhandle shaderObj, GLsizei count,
                             const GLchar **string, const GLint *length );
typedef void (APIENTRY
    *GL_COMPILE_SHADER_FPTR)( GLhandle shaderObj );
typedef GLhandle (APIENTRY
    *GL_CREATE_PROGRAM_FPTR)( void );
typedef void (APIENTRY
    *GL_ATTACH_SHADER_FPTR)( GLhandle program, GLhandle shader );
typedef void (APIENTRY
    *GL_BIND_ATTRIB_LOCATION_FPTR)( GLhandle program, GLint loc, GLchar *name );
typedef void (APIENTRY
    *GL_LINK_PROGRAM_FPTR)( GLhandle program );
typedef void (APIENTRY
    *GL_USE_PROGRAM_FPTR)( GLhandle program );

#define GL_NUM_EXTENSIONS       0x821D
typedef const GLubyte *(APIENTRY
    *GL_GET_STRING_I_FPTR)( GLenum name, GLuint index );

extern GL_GET_SHADER_IV_FPTR            glGetShaderiv;
extern GL_GET_PROGRAM_IV_FPTR           glGetProgramiv;
extern GL_GET_SHADER_SOURCE_FPTR        glGetShaderSource;
extern GL_GET_SHADER_INFO_LOG_FPTR      glGetShaderInfoLog;
extern GL_GET_PROGRAM_INFO_LOG_FPTR     glGetProgramInfoLog;
extern GL_GET_UNIFORM_LOCATION_FPTR     glGetUniformLocation;
extern GL_UNIFORM_1FV_FPTR              glUniform1fv;
extern GL_UNIFORM_2FV_FPTR              glUniform2fv;
extern GL_UNIFORM_3FV_FPTR              glUniform3fv;
extern GL_UNIFORM_4FV_FPTR              glUniform4fv;
extern GL_UNIFORM_1I_FPTR               glUniform1i;
extern GL_UNIFORM_MATRIX_3FV_FPTR       glUniformMatrix3fv;
extern GL_UNIFORM_MATRIX_4FV_FPTR       glUniformMatrix4fv;
extern GL_CREATE_SHADER_FPTR            glCreateShader;
extern GL_SHADER_SOURCE_FPTR            glShaderSource;
extern GL_COMPILE_SHADER_FPTR           glCompileShader;
extern GL_CREATE_PROGRAM_FPTR           glCreateProgram;
extern GL_ATTACH_SHADER_FPTR            glAttachShader;
extern GL_BIND_ATTRIB_LOCATION_FPTR     glBindAttribLocation;
extern GL_LINK_PROGRAM_FPTR             glLinkProgram;
extern GL_USE_PROGRAM_FPTR              glUseProgram;
extern GL_GET_STRING_I_FPTR             glGetStringi;

/***************************************************************************
 * Vertex attribs
 ***************************************************************************/
typedef void (APIENTRY
    *GL_VERTEX_ATTRIB_POINTER_FPTR)( GLuint index, GLint size, GLenum type,
                GLboolean normalized, GLsizei stride, const GLvoid *pointer);
typedef void (APIENTRY
    *GL_ENABLE_VERTEX_ATTRIB_ARRAY_FPTR)( GLuint index );
typedef void (APIENTRY
    *GL_DISABLE_VERTEX_ATTRIB_ARRAY_FPTR)( GLuint index );
typedef void (APIENTRY
    *GL_VERTEX_ATTRIB_4F_FPTR)( GLuint index,
                                GLfloat x, GLfloat y, GLfloat z, GLfloat w);

extern GL_VERTEX_ATTRIB_POINTER_FPTR            glVertexAttribPointer;
extern GL_ENABLE_VERTEX_ATTRIB_ARRAY_FPTR       glEnableVertexAttribArray;
extern GL_DISABLE_VERTEX_ATTRIB_ARRAY_FPTR      glDisableVertexAttribArray;
extern GL_VERTEX_ATTRIB_4F_FPTR                 glVertexAttrib4f;
#endif /* ] */

/* Define extension bits for what OpenGL version we support.  */
#define HWGL_EXT_OPENGL_1_1     0x00000001
#define HWGL_EXT_OPENGL_2_0     0x00000002
#define HWGL_EXT_OPENGL_3_0     0x00000004
#define HWGL_EXT_OPENGL_4_0     0x00000008
