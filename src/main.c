#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/gl.h>
#include <stdio.h>

#include "platform.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "glcore.h"

GLuint enemies_vao = 0, enemies_vbo = 0, enemies_ebo = 0, enemies_program = 0;

LARGE_INTEGER frequencey;
LARGE_INTEGER previous_time;
#define TARGET_FPS 60
#define TARGET_FRAME_TIME (1.0f / TARGET_FPS)  // = ~0.03333 seconds

#define ALIEN_COLS 3
#define ALIEN_ROWS 7
#define ALIEN_HEIGHT 40
#define ALIEN_WIDTH 40

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

#define X(name, type) \
    name = (type)wglGetProcAddress(#name); \
    if (!name) { \
        fprintf(stderr, "Failed to load OpenGL function: %s\n", #name); \
        return -1; \
    }

typedef struct {
    float u;     // left
    float v;     // top
    float width; // width in UV
    float height;// height in UV
} UVRect;

/*void draw_debug_quad();*/
void draw_enemy_sprite(float x, float y, GLuint texture, int col, int row);
UVRect get_sprite_uv(int col, int row);
float pixels_to_ndc_x(float px);
float pixels_to_ndc_y(float px);

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

int LoadOpenGL(void)
{
    X(glGenVertexArrays, PFNGLGENVERTEXARRAYSPROC);
    X(glBindVertexArray, PFNGLBINDVERTEXARRAYPROC);
    X(glGenBuffers, PFNGLGENBUFFERSPROC);
    X(glBindBuffer, PFNGLBINDBUFFERPROC);
    X(glBufferData, PFNGLBUFFERDATAPROC);
    X(glCreateShader, PFNGLCREATESHADERPROC);
    X(glShaderSource, PFNGLSHADERSOURCEPROC);
    X(glCompileShader, PFNGLCOMPILESHADERPROC);
    X(glCreateProgram, PFNGLCREATEPROGRAMPROC);
    X(glAttachShader, PFNGLATTACHSHADERPROC);
    X(glLinkProgram, PFNGLLINKPROGRAMPROC);
    X(glUseProgram, PFNGLUSEPROGRAMPROC);
    X(glVertexAttribPointer, PFNGLVERTEXATTRIBPOINTERPROC);
    X(glEnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAYPROC);
    X(glGetShaderInfoLog, PFNGLGETSHADERINFOLOGPROC);
    X(glGetShaderiv, PFNGLGETSHADERIVPROC);
    X(glGetProgramiv, PFNGLGETPROGRAMIVPROC);
    X(glGetProgramInfoLog, PFNGLGETPROGRAMINFOLOGPROC);
    X(glUniform2f, PFNGLUNIFORM2FPROC);
    X(glGetUniformLocation, PFNGLGETUNIFORMLOCATIONPROC);
    X(glActiveTexture, PFNGLACTIVETEXTUREPROC);
    X(glGenerateMipmap, PFNGLGENERATEMIPMAPPROC);
    X(glUniform1i, PFNGLUNIFORM1IPROC);
    X(glDeleteShader, PFNGLDELETESHADERPROC);


    return 0;
}

#define MAX_BULLETS 100
typedef struct {
    float x, y;
    int active;
} Bullet;
Bullet bullets[MAX_BULLETS];

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    // 1. Register window class
    WNDCLASS wc = {0};
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "OpenGLWindowClass";
    RegisterClass(&wc);

    // 2. Create window
    HWND hwnd = CreateWindowEx(0, wc.lpszClassName, "OpenGL Window",
                               WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                               CW_USEDEFAULT, CW_USEDEFAULT, SCREEN_WIDTH, SCREEN_HEIGHT,
                               NULL, NULL, hInstance, NULL);

    HDC hdc = GetDC(hwnd);

    // 3. Set temporary pixel format for dummy context
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR), 1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA, 32, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0,
        24, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0
    };

    int pf = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pf, &pfd);

    // 4. Create dummy OpenGL context and make it current
    HGLRC dummyRC = wglCreateContext(hdc);
    wglMakeCurrent(hdc, dummyRC);

    // 5. Load wglCreateContextAttribsARB manually
    wglCreateContextAttribsARBProc wglCreateContextAttribsARB = (wglCreateContextAttribsARBProc)
        wglGetProcAddress("wglCreateContextAttribsARB");

    if (!wglCreateContextAttribsARB) {
        MessageBox(NULL, "wglCreateContextAttribsARB not supported", "Error", MB_OK);
        return -1;
    }

    // 6. Ask for OpenGL 4.4 Core Profile context
    int attribs[] = {
        0x2091, 4,    // WGL_CONTEXT_MAJOR_VERSION_ARB = 0x2091
        0x2092, 4,    // WGL_CONTEXT_MINOR_VERSION_ARB = 0x2092
        0x9126, 0x00000001, // WGL_CONTEXT_PROFILE_MASK_ARB = CORE
        0
    };

    HGLRC realRC = wglCreateContextAttribsARB(hdc, 0, attribs);

    // 7. Switch to real context
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(dummyRC);
    wglMakeCurrent(hdc, realRC);

    // Load gl functions
    if (LoadOpenGL() != 0)
    {
        fprintf(stderr, "Could not load all of the opengl functions.\n");
        return -1;
    }

    // ship vao/vbo setup
    GLuint vao_ship;
    GLuint ship_vbo;
    glGenVertexArrays(1, &vao_ship);
    glBindVertexArray(vao_ship);
    float ship_vertices[] = {
         0.0f,  0.1f,
        -0.025f, -0.025f,
         0.025f, -0.025f
    };
    glGenBuffers(1, &ship_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, ship_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ship_vertices), ship_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);


    // bullets vao/vbo setup
    GLuint bullets_vbo, bullets_vao;
    glGenBuffers(1, &bullets_vbo);
    glGenVertexArrays(1, &bullets_vao);
    glGenBuffers(1, &bullets_vbo);
    glBindVertexArray(bullets_vao);
    glBindBuffer(GL_ARRAY_BUFFER, bullets_vbo);
    // We don't upload data here — we’ll update per bullet in the draw loop
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // bullet and ship shader shaders and program
    File vert_shader = os_read_file((u8*)"./assets/shaders/vert.glsl");
    File frag_shader = os_read_file((u8*)"./assets/shaders/frag.glsl");
    const char* vert_shader_src = (const char*)vert_shader.data;
    const char* frag_shader_src = (const char*)frag_shader.data;
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vert_shader_src, NULL);
    glCompileShader(vs);
    char log[512];
    GLint success;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vs, 512, NULL, log);
        MessageBoxA(NULL, log, "vertex shader compile error\n", MB_OK);
    }
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &frag_shader_src, NULL);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fs, 512, NULL, log);
        MessageBoxA(NULL, log, "fragment shader compile error\n", MB_OK);
    }
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, log);
        MessageBoxA(NULL, log, "Shader Program Linking Error", MB_OK);
        return -1;
    }
    glUseProgram(program);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // ----------- setup enemies
    File texture_vert_shader = os_read_file((u8*)"./assets/shaders/texture_vert.glsl");
    File texture_frag_shader = os_read_file((u8*)"./assets/shaders/texture_frag.glsl");
    const char* texture_vert_shader_src = (const char*)texture_vert_shader.data;
    const char* texture_frag_shader_src = (const char*)texture_frag_shader.data;
    GLuint tvs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(tvs, 1, &texture_vert_shader_src, NULL);
    glCompileShader(tvs);
    char texture_log[512];
    GLint texture_success;
    glGetShaderiv(tvs, GL_COMPILE_STATUS, &texture_success);
    if (!texture_success) {
        glGetShaderInfoLog(tvs, 512, NULL, texture_log);
        MessageBoxA(NULL, texture_log, "Texture vertex shader compile error\n", MB_OK);
    }
    GLuint tfs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(tfs, 1, &texture_frag_shader_src, NULL);
    glCompileShader(tfs);
    if (!texture_success) {
        glGetShaderInfoLog(tfs, 512, NULL, texture_log);
        MessageBoxA(NULL,texture_log, "Texture fragment shader compile error\n", MB_OK);
    }
    enemies_program = glCreateProgram();
    glAttachShader(enemies_program, tvs);
    glAttachShader(enemies_program, tfs);
    glLinkProgram(enemies_program);

    glDeleteShader(tvs);
    glDeleteShader(tfs);

    // this is scaling the quad due to how my sprite sheet is setup
    // it isn't always necessary. It depends on the texture.
    f32 quad_width = pixels_to_ndc_x(ALIEN_WIDTH);
    f32 quad_height = pixels_to_ndc_y(ALIEN_HEIGHT); // if vert padding is fine don't scale

    f32 vertices[] = {
        // x, y      u, v
        -quad_width, -quad_height,  0.0f, 1.0f,
         quad_width, -quad_height,  1.0f, 1.0f,
         quad_width,  quad_height,  1.0f, 0.0f,
        -quad_width,  quad_height,  0.0f, 0.0f
    };
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };
    glGenVertexArrays(1, &enemies_vao);
    glGenBuffers(1, &enemies_vbo);
    glGenBuffers(1, &enemies_ebo);
    glBindVertexArray(enemies_vao);
    glBindBuffer(GL_ARRAY_BUFFER, enemies_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, enemies_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);                     // aPos
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));  // aUV
    glEnableVertexAttribArray(1);
    // ----------- setup enemies end


    // @TODO: put this into a gfx_load_texture() function
    int width, height, channels;
    /*stbi_set_flip_vertically_on_load(1);*/

    const char * sprite_sheet_path = "./assets/images/space_invaders_sprite_sheet.png";
    unsigned char* data = stbi_load(sprite_sheet_path, &width, &height, &channels, 4);
    if (!data)
    {
        fprintf(stderr, "failed to load %s", sprite_sheet_path);
        return 1;
    }
    fprintf(stderr, "Channels: %d", channels);

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


    GLint uOffset = glGetUniformLocation(program, "uOffset");
    float xPos = 0.0f;
    float yPos = -0.8f;

    float alienGroupOffsetX = 0.0f;
    float alienGroupOffsetY = 0.0f;
    f32 alienGroupDirection = 1.0f;
    f32 alienGroupSpeed = 0.2f;

    float alienAnimTimer = 0.0f;
    const float alienAnimInterval = 0.5f;  // change frame every 0.5s
    int alienAnimFrame = 0;

    QueryPerformanceFrequency(&frequencey);
    QueryPerformanceCounter(&previous_time);

    float groupWidth = 10 * .1; // depends on spacing
    float groupLeft = -0.6f + alienGroupOffsetX;
    float groupRight = groupLeft + groupWidth;

    // 8. Main loop
    MSG msg;
    while (1) {
        LARGE_INTEGER current_time;
        QueryPerformanceCounter(&current_time);
        f32 deltaTime = (f32)(current_time.QuadPart - previous_time.QuadPart) / frequencey.QuadPart;
        previous_time = current_time;

        // FPS
        LARGE_INTEGER freq, frameStart, frameEnd;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&frameStart);  // at top of loop

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT)
                return 0;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // move player left and right
        if (GetAsyncKeyState(VK_LEFT) & 0x8000)
        {
            xPos -= 0.01f;
        }
        if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
        {
            xPos += 0.01f;
        }

        if (xPos < -0.90f)
        {
            xPos = -0.9f;
        }
        if (xPos > 0.90f)
        {
            xPos = 0.9f;
        }

        // define teh bullets
        static SHORT lastSpace = 0;
        SHORT currentSpace = GetAsyncKeyState(VK_SPACE);
        if ((currentSpace & 0x8000) && !(lastSpace & 0x8000))
        {
            for (int i = 0; i < MAX_BULLETS; ++i)
            {
                if (!bullets[i].active)
                {
                    bullets[i].x = xPos;
                    bullets[i].y = -0.7f; // from the base of the triangle
                    bullets[i].active = 1;
                    break;
                }
            }
        }
        lastSpace = currentSpace;

        // move the bullets
        f32 bullet_speed = 0.04f;
        for(int i = 0; i < MAX_BULLETS; ++i)
        {
            if (bullets[i].active)
            {
                bullets[i].y += bullet_speed;
                if (bullets[i].y > 1.1f) bullets[i].active = 0;
            }
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(program);

        // draw the bullets
        // note that you have to bind the vertex that you want to draw
        glUniform2f(uOffset, 0.0f, 0.0f);
        glBindVertexArray(bullets_vao);
        for (int i = 0; i < MAX_BULLETS; ++i)
        {
            if (!bullets[i].active) continue;

            float bullet_verts[] = {
                bullets[i].x, bullets[i].y,
                bullets[i].x, bullets[i].y + 0.05f
            };

            glBindBuffer(GL_ARRAY_BUFFER, bullets_vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(bullet_verts), bullet_verts, GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 *sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glDrawArrays(GL_LINES, 0, 2);
        }

        // draw ship
        glBindVertexArray(vao_ship);
        glUniform2f(uOffset, xPos, -0.8f);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        for (int row = 0; row < 5; ++row) {
            for (int col = 0; col < 10; ++col) {

                f32 column_gap = pixels_to_ndc_x(ALIEN_WIDTH * 2.f);
                f32 row_gap = pixels_to_ndc_y(ALIEN_HEIGHT * 1.75f);

                f32 xoffset = pixels_to_ndc_x(-400);
                f32 yoffset = pixels_to_ndc_y(500);

                float x = (xoffset + col * column_gap);
                float y = (yoffset - row * row_gap);

                draw_enemy_sprite(x, y, aliens_texture, 0, 0);
            }
        }

        SwapBuffers(hdc);

        QueryPerformanceCounter(&frameEnd);  // at bottom of loop

        float frameTime = (float)(frameEnd.QuadPart - frameStart.QuadPart) / freq.QuadPart;
        float sleepTime = TARGET_FRAME_TIME - frameTime;
        if (sleepTime > 0) {
            Sleep((DWORD)(sleepTime * 1000.0f));
        }
    }

    return 0;
}

void draw_enemy_sprite(float x, float y, GLuint texture, int col, int row)
{
    UVRect sprite = get_sprite_uv(col, row);

    glUseProgram(enemies_program);
    glUniform2f(glGetUniformLocation(enemies_program, "uOffset"), x, y);

    glUniform2f(glGetUniformLocation(enemies_program, "uUVOffset"), sprite.u, sprite.v);
    glUniform2f(glGetUniformLocation(enemies_program, "uUVScale"), sprite.width, sprite.height);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(enemies_program, "texture1"), 0);

    glBindVertexArray(enemies_vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

/*void draw_debug_quad() {*/
/*    static GLuint vao = 0, vbo = 0, ebo = 0, program = 0;*/
/**/
/*    if (program == 0) {*/
/*        // === SHADERS ===*/
/*        const char* vsSrc = "#version 330 core\n"*/
/*            "layout(location = 0) in vec2 aPos;\n"*/
/*            "void main() {\n"*/
/*            "  gl_Position = vec4(aPos, 0.0, 1.0);\n"*/
/*            "}";*/
/**/
/*        const char* fsSrc = "#version 330 core\n"*/
/*            "out vec4 FragColor;\n"*/
/*            "void main() {\n"*/
/*            "  FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"*/
/*            "}";*/
/**/
/*        GLuint vs = glCreateShader(GL_VERTEX_SHADER);*/
/*        glShaderSource(vs, 1, &vsSrc, NULL);*/
/*        glCompileShader(vs);*/
/**/
/*        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);*/
/*        glShaderSource(fs, 1, &fsSrc, NULL);*/
/*        glCompileShader(fs);*/
/**/
/*        program = glCreateProgram();*/
/*        glAttachShader(program, vs);*/
/*        glAttachShader(program, fs);*/
/*        glLinkProgram(program);*/
/**/
/*        glDeleteShader(vs);*/
/*        glDeleteShader(fs);*/
/**/
/*        // === VERTEX DATA ===*/
/*        float vertices[] = {*/
/*            -0.1f, -0.1f,  // bottom-left*/
/*             0.1f, -0.1f,  // bottom-right*/
/*             0.1f,  0.1f,  // top-right*/
/*            -0.1f,  0.1f   // top-left*/
/*        };*/
/**/
/*        unsigned int indices[] = {*/
/*            0, 1, 2,*/
/*            2, 3, 0*/
/*        };*/
/**/
/*        glGenVertexArrays(1, &vao);*/
/*        glGenBuffers(1, &vbo);*/
/*        glGenBuffers(1, &ebo);*/
/**/
/*        glBindVertexArray(vao);*/
/*        glBindBuffer(GL_ARRAY_BUFFER, vbo);*/
/*        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);*/
/**/
/*        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);*/
/*        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);*/
/**/
/*        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);*/
/*        glEnableVertexAttribArray(0);*/
/*    }*/
/**/
/*    // === DRAW ===*/
/*    glUseProgram(program);*/
/*    glBindVertexArray(vao);*/
/*    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);*/
/*}*/

UVRect get_sprite_uv(int col, int row) {
    UVRect r;
    float spriteW = 1.0f / (float)ALIEN_COLS;
    float spriteH = 1.0f / (float)ALIEN_ROWS;

    r.u = col * spriteW;
    r.v = 1.0f - (row + 1) * spriteH;
    r.width = spriteW;
    r.height = spriteH;

    return r;
}

float pixels_to_ndc_x(float px)
{
    return px * (2.0f / (float)SCREEN_WIDTH);
}

float pixels_to_ndc_y(float px)
{
    return px * (2.0f / (float)SCREEN_HEIGHT);
}
