#include "glcore.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "base_types.h"
#include "platform.h"


#define X(type, name) \
    name = (type)wglGetProcAddress(#name); \
    if (!name) { \
        fprintf(stderr, "Failed to load OpenGL function: %s\n", #name); \
        return -1; \
    }

PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = 0;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray = 0;
PFNGLGENBUFFERSPROC glGenBuffers = 0;
PFNGLBINDBUFFERPROC glBindBuffer = 0;
PFNGLBUFFERDATAPROC glBufferData = 0;
PFNGLCREATESHADERPROC glCreateShader = 0;
PFNGLSHADERSOURCEPROC glShaderSource = 0;
PFNGLCOMPILESHADERPROC glCompileShader = 0;
PFNGLCREATEPROGRAMPROC glCreateProgram = 0;
PFNGLATTACHSHADERPROC glAttachShader = 0;
PFNGLLINKPROGRAMPROC glLinkProgram = 0;
PFNGLUSEPROGRAMPROC glUseProgram = 0;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = 0;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = 0;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = 0;
PFNGLGETSHADERIVPROC glGetShaderiv = 0;
PFNGLGETPROGRAMIVPROC glGetProgramiv = 0;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = 0;
PFNGLUNIFORM2FPROC glUniform2f = 0;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = 0;
PFNGLACTIVETEXTUREPROC glActiveTexture = 0;
PFNGLGENERATEMIPMAPPROC glGenerateMipmap = 0;
PFNGLUNIFORM1IPROC glUniform1i = 0;
PFNGLDELETESHADERPROC glDeleteShader = 0;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = 0;
PFNGLUNIFORM1FPROC glUniform1f = 0;
PFNGLUNIFORM4FPROC glUniform4f = 0;
PFNGLBUFFERSUBDATAPROC glBufferSubData = 0;

void gfx_init_quad(GLuint* vao_out, GLuint* vbo_out, GLuint* ebo_out)
{
    float quadVerts[] = {
        // pos      // uv
        -1.0f, -1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 1.0f,
         1.0f,  1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 0.0f
    };

    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    if (vao_out) *vao_out = vao;
    if (vbo_out) *vbo_out = vbo;
    if (ebo_out) *ebo_out = ebo;
}

GLuint gfx_create_texture(const char *image_path)
{
    int width, height, channels;
    stbi_set_flip_vertically_on_load(1);
    unsigned char* data = stbi_load(image_path, &width, &height, &channels, 4);
    if (!data)
    {
        fprintf(stderr, "failed to load %s", image_path);
        return FALSE;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint aliens_texture;
    glGenTextures(1, &aliens_texture);
    glBindTexture(GL_TEXTURE_2D, aliens_texture);

    // set filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // upload image to GPU
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);

    return aliens_texture;
}

GLuint gfx_create_shader_program(const char* vert_shader_path, const char* frag_shader_path)
{
    File vert_shader = os_read_file((u8*)vert_shader_path);
    File frag_shader = os_read_file((u8*)frag_shader_path);
    const char* vert_shader_src = (const char*)vert_shader.data;
    const char* frag_shader_src = (const char*)frag_shader.data;
    GLuint tvs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(tvs, 1, &vert_shader_src, NULL);
    glCompileShader(tvs);
    char texture_log[512];
    GLint texture_success;
    glGetShaderiv(tvs, GL_COMPILE_STATUS, &texture_success);
    if (!texture_success) {
        glGetShaderInfoLog(tvs, 512, NULL, texture_log);
        MessageBoxA(NULL, texture_log, "Texture vertex shader compile error\n", MB_OK);
    }
    GLuint tfs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(tfs, 1, &frag_shader_src, NULL);
    glCompileShader(tfs);
    if (!texture_success) {
        glGetShaderInfoLog(tfs, 512, NULL, texture_log);
        MessageBoxA(NULL,texture_log, "Texture fragment shader compile error\n", MB_OK);
    }
    GLuint program = glCreateProgram();
    glAttachShader(program, tvs);
    glAttachShader(program, tfs);
    glLinkProgram(program);

    glDeleteShader(tvs);
    glDeleteShader(tfs);

    return program;
}

int load_gl_functions(void)
{
    X(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays);
    X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray);
    X(PFNGLGENBUFFERSPROC, glGenBuffers);
    X(PFNGLBINDBUFFERPROC, glBindBuffer);
    X(PFNGLBUFFERDATAPROC, glBufferData);
    X(PFNGLCREATESHADERPROC, glCreateShader);
    X(PFNGLSHADERSOURCEPROC, glShaderSource);
    X(PFNGLCOMPILESHADERPROC, glCompileShader);
    X(PFNGLCREATEPROGRAMPROC, glCreateProgram);
    X(PFNGLATTACHSHADERPROC, glAttachShader);
    X(PFNGLLINKPROGRAMPROC, glLinkProgram);
    X(PFNGLUSEPROGRAMPROC, glUseProgram);
    X(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer);
    X(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray);
    X(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog);
    X(PFNGLGETSHADERIVPROC, glGetShaderiv);
    X(PFNGLGETPROGRAMIVPROC, glGetProgramiv);
    X(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog);
    X(PFNGLUNIFORM2FPROC, glUniform2f);
    X(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation);
    X(PFNGLACTIVETEXTUREPROC, glActiveTexture);
    X(PFNGLGENERATEMIPMAPPROC, glGenerateMipmap);
    X(PFNGLUNIFORM1IPROC, glUniform1i);
    X(PFNGLDELETESHADERPROC, glDeleteShader);
    X(PFNGLUNIFORMMATRIX4FV, glUniformMatrix4fv);
    X(PFNGLUNIFORM1FPROC, glUniform1f);
    X(PFNGLUNIFORM4FPROC, glUniform4f);
    X(PFNGLBUFFERSUBDATAPROC, glBufferSubData);

    return 0;
}

