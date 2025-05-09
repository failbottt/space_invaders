#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/gl.h>
#include <stdio.h>

#include "platform.h"
#include "handmade_math.h"

#include "glcore.h"
#include "gfx.h"

GLuint enemies_vao = 0, enemies_vbo = 0, enemies_ebo = 0, enemies_program = 0;

LARGE_INTEGER frequencey;
LARGE_INTEGER previous_time;
#define TARGET_FPS 60
#define TARGET_FRAME_TIME (1.0f / TARGET_FPS)  // = ~0.03333 seconds

#define SPRITE_COLS 3
#define SPRITE_ROWS 7
#define SPRITE_SHEET_WIDTH 1024
#define SPRITE_SHEET_HEIGHT 1536

#define SPRITE_WIDTH (f32)((f32)SPRITE_SHEET_WIDTH / SPRITE_COLS)
#define SPRITE_HEIGHT (f32)((f32)SPRITE_SHEET_HEIGHT / SPRITE_ROWS)

int NUM_COLS = 10;
int NUM_ROWS = 5;

typedef struct {
    f32 x, y;
    f32 height, width;
    u8 alive;
}Alien;

#define MAX_ALIENS 50

Alien aliens[MAX_ALIENS];

int SCREEN_WIDTH = 0;
int SCREEN_HEIGHT = 0;

typedef struct {
    float u0, v0; // top-left
    float u1, v1; // bottom-right
} UVRect;

/*void draw_debug_quad();*/
void draw_enemy_sprite(float x, float y, GLuint texture, int col, int row, HMM_Mat4 projection);
UVRect get_sprite_uv(int col, int row);
void draw_bullet(GLuint program, GLuint vao, float x, float y, float size, HMM_Mat4 projection);

u8 check_aabb_collision(float ax, float ay, float aw, float ah,
                          float bx, float by, float bw, float bh)
{
    return ax < bx + bw &&
           ax + aw > bx &&
           ay < by + bh &&
           ay + ah > by;
}



#define MAX_BULLETS 100
typedef struct {
    u8 active;
    f32 x;
    f32 y;
    f32 velocity;
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
                               CW_USEDEFAULT, CW_USEDEFAULT, 1920, 1080,
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
    if (load_gl_functions() != 0)
    {
        fprintf(stderr, "Could not load all of the opengl functions.\n");
        return -1;
    }

    RECT rect;
    GetClientRect(hwnd, &rect);
    SCREEN_WIDTH = rect.right - rect.left;
    SCREEN_HEIGHT = rect.bottom - rect.top;
    // glViewPort here

    // ship vao/vbo setup
    GLuint vao_ship;
    GLuint ship_vbo;
    glGenVertexArrays(1, &vao_ship);
    glBindVertexArray(vao_ship);

    float ship_vertices[] = {
        0.0f, -1.0f,   // bottom center
       -1.0f,  1.0f,   // top-left
        1.0f,  1.0f    // top-right
    };
    glGenBuffers(1, &ship_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, ship_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ship_vertices), ship_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    float bulletVerts[] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        1.0f,  1.0f,
        -1.0f,  1.0f
    };

    unsigned int bulletIndices[] = {
        0, 1, 2,
        2, 3, 0
    };


    // bullets vao/vbo setup
    GLuint bullets_vbo, bullets_vao, bullets_ebo;
    glGenVertexArrays(1, &bullets_vao);
    glGenBuffers(1, &bullets_vbo);
    glGenBuffers(1, &bullets_ebo);
    glBindVertexArray(bullets_vao);
    glBindBuffer(GL_ARRAY_BUFFER, bullets_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bulletVerts), bulletVerts, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bullets_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(bulletIndices), bulletIndices, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);


    // bullet and ship shader shaders and program
    GLuint program = gfx_create_shader_program(
            (const char*)"./assets/shaders/vert.glsl",
            (const char*)"./assets/shaders/frag.glsl"
            );

    HMM_Mat4 projection_mat = HMM_Orthographic_RH_ZO(
            0.0f, (float)SCREEN_WIDTH,         // left → right
            (float)SCREEN_HEIGHT, 0.0f,        // bottom → top (Y flipped to match top-left origin)
            -1.0f, 1.0f                       // near/far
            );

    glUseProgram(program);
    glUniformMatrix4fv(glGetUniformLocation(program, "uProjection"), 1, GL_FALSE, (float*)&projection_mat);
    glUniform2f(glGetUniformLocation(program, "uOffset"), 700.f, 512.f);
    glUniform1f(glGetUniformLocation(program, "uScale"), 32.0f); // 64px triangle (since triangle ranges -1 to +1)


    enemies_program = gfx_create_shader_program(
            (const char*)"./assets/shaders/texture_vert.glsl",
            (const char*)"./assets/shaders/texture_frag.glsl"
            );

    gfx_init_quad(&enemies_vao, &enemies_vbo, &enemies_ebo);

    GLuint aliens_texture = gfx_create_texture((const char*)"./assets/images/space_invaders_sprite_sheet.png");

    float ship_x_pos = 200.f;
    float ship_y_pos = (f32)SCREEN_HEIGHT - 40.f;

    QueryPerformanceFrequency(&frequencey);
    QueryPerformanceCounter(&previous_time);

    // init bullets
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        bullets[i].active = FALSE;
    }

    float alienGroupOffsetX = 0.0f;
    float alienGroupOffsetY = 0.0f;
    float alien_move_speed = 60.0f;     // pixels per second
    int alienMoveDirection = 1;       // +1 = right, -1 = left
    u8 alien_should_drop = 0;
                                      //
    f32 ALIEN_SPACING_X = (SPRITE_WIDTH / 4 + 8);
    f32 total_grid_width = NUM_COLS * ALIEN_SPACING_X;
    f32 startX = (SCREEN_WIDTH - total_grid_width) / 2;

    f32 ALIEN_SPACING_Y = (SPRITE_HEIGHT / 4 + 8);
    f32 total_grid_height = NUM_COLS * ALIEN_SPACING_Y;
    f32 startY = ((SCREEN_HEIGHT - total_grid_height) / 2.0f) - 150.f;

     for (int row = 0; row < NUM_ROWS; row++) {
        for (int col = 0; col < NUM_COLS; col++) {
            Alien* a = &aliens[row * NUM_COLS + col];
            a->x = startX + col * ALIEN_SPACING_X;
            a->y = startY + row * ALIEN_SPACING_Y;
            a->alive = TRUE;
            a->width = SPRITE_WIDTH * .1f;
            a->height = SPRITE_HEIGHT * .1f;
        }
    }

    // 8. Main loop
    MSG msg;
    u8 running = 1;
    while (running) {
        LARGE_INTEGER current_time;
        QueryPerformanceCounter(&current_time);
        f32 deltaTime = (f32)(current_time.QuadPart - previous_time.QuadPart) / frequencey.QuadPart;
        if (deltaTime > 0.05f)
        {
            deltaTime = 0.05f;
        }
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
            ship_x_pos -= 7.f;
        }
        if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
        {
            ship_x_pos += 7.f;
        }
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
        {
            running = 0;
        }

        if (ship_x_pos < 40.f)
        {
            ship_x_pos = 40.f;
        }
        if (ship_x_pos > (float)SCREEN_WIDTH - 120.f)
        {
            ship_x_pos = (float)SCREEN_WIDTH - 120.f;
        }

        // fetch input for bullets
        static SHORT lastSpace = 0;
        SHORT currentSpace = GetAsyncKeyState(VK_SPACE);
        if ((currentSpace & 0x8000) && !(lastSpace & 0x8000))
        {
            for (int i = 0; i < MAX_BULLETS; ++i)
            {
                if (!bullets[i].active)
                {
                    bullets[i].x = ship_x_pos;
                    bullets[i].y = ship_y_pos; // from the base of the triangle
                    bullets[i].velocity = -5.0f * deltaTime;
                    bullets[i].active = TRUE;
                    break;
                }
            }
        }
        lastSpace = currentSpace;

        // update bullets
        for (int i = 0; i < MAX_BULLETS; i++)
        {
            if (!bullets[i].active) continue;

            bullets[i].y -= 10.0f;

            if (bullets[i].y < -16.f)
            {
                bullets[i].active = FALSE;
            }
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(program);

        // draw ship
        glBindVertexArray(vao_ship);
        glUniform2f(glGetUniformLocation(program, "uOffset"), ship_x_pos, ship_y_pos);  // position in pixels
        glUniform1f(glGetUniformLocation(program, "uScale"), 32.0f);
        glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, (float*)&projection_mat);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            fprintf(stderr, "GL error: %d\n", err);
        }

        for (int i = 0; i < MAX_BULLETS; i++)
        {
            if (!bullets[i].active) continue;

            draw_bullet(program, bullets_vao, bullets[i].x, bullets[i].y, 8.0f, projection_mat);
        }

        {
            // check bullet collision
            for (int b = 0; b < MAX_BULLETS; b++) {
                if (!bullets[b].active) continue;

                float bx = bullets[b].x - 4 * 0.5f;
                float by = bullets[b].y - 16 * 0.5f;

                for (int a = 0; a < NUM_ROWS * NUM_COLS; a++) {
                    if (!aliens[a].alive) continue;

                    float ax = aliens[a].x + alienGroupOffsetX - aliens[0].width;
                    float ay = aliens[a].y + alienGroupOffsetY - aliens[0].height;

                    u8 bullet_has_collided = check_aabb_collision(ax, ay, aliens[0].width, aliens[0].height, bx, by, 4, 16);
                    if (bullet_has_collided)
                    {
                        aliens[a].alive = FALSE;
                        bullets[b].active = FALSE;

                        // Optional: score++, sound, flash, etc.
                        break;  // stop checking this bullet after one hit
                    }
                }
            }
        }

        {
            // update and draw aliens
            // Find bounding edges of the living aliens
            float firstAlienX = INFINITY;
            float lastAlienX = -INFINITY;

            for (int i = 0; i < NUM_ROWS * NUM_COLS; i++) {
                if (!aliens[i].alive) continue;
                if (aliens[i].x < firstAlienX) firstAlienX = aliens[i].x;
                if (aliens[i].x > lastAlienX)  lastAlienX = aliens[i].x;
            }

            // Move horizontally
            alienGroupOffsetX += alien_move_speed * alienMoveDirection * deltaTime;

            if (!alien_should_drop)
            {
                // Check for collision with screen edge
                float leftEdge = alienGroupOffsetX + firstAlienX;
                float rightEdge = alienGroupOffsetX + lastAlienX + aliens[0].width;

                if (leftEdge < 20 || rightEdge > SCREEN_WIDTH - 20) {
                    // Reverse direction
                    alienMoveDirection *= -1;
                    alien_should_drop = TRUE;
                }
            }

            if (alien_should_drop)
            {
                alienGroupOffsetY += aliens[0].height;
                alien_should_drop = FALSE;
            }

            for (int i = 0; i < NUM_ROWS * NUM_COLS; i++) {
                if (!aliens[i].alive) continue;

                float x = aliens[i].x + alienGroupOffsetX;
                float y = aliens[i].y + alienGroupOffsetY;


                draw_enemy_sprite(x, y, aliens_texture, 0, 0, projection_mat);
            }
        }


        SwapBuffers(hdc);

        {
            // fps/sleep
            QueryPerformanceCounter(&frameEnd);  // at bottom of loop
            float frameTime = (float)(frameEnd.QuadPart - frameStart.QuadPart) / freq.QuadPart;
            float sleepTime = TARGET_FRAME_TIME - frameTime;
            if (sleepTime > 0) {
                Sleep((DWORD)(sleepTime * 1000.0f));
            }
        }
    }

    return 0;
}

void draw_enemy_sprite(float x, float y, GLuint texture, int col, int row, HMM_Mat4 projection)
{
    UVRect uv = get_sprite_uv(col, row);

    float verts[] = {
        // pos       // uv
        -1.0f, -1.0f,   uv.u0, uv.v1,
         1.0f, -1.0f,   uv.u1, uv.v1,
         1.0f,  1.0f,   uv.u1, uv.v0,
        -1.0f,  1.0f,   uv.u0, uv.v0
    };

    glBindBuffer(GL_ARRAY_BUFFER, enemies_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

    glUseProgram(enemies_program);
    glUniformMatrix4fv(glGetUniformLocation(enemies_program, "uProjection"), 1, GL_FALSE, (float*)&projection);
    glUniform2f(glGetUniformLocation(enemies_program, "uOffset"), x, y);                     // center position in pixels
    glUniform2f(glGetUniformLocation(enemies_program, "uSize"), aliens[0].width, aliens[0].height); // half-size in pixels
    glUniform4f(glGetUniformLocation(enemies_program, "uColor"), 1.0f, 1.0f, 1.0f, 1.0f);    // no tint


    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(enemies_program, "texture1"), 0);

    glBindVertexArray(enemies_vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

UVRect get_sprite_uv(int col, int row)
{
    float uSize = 1.0f / SPRITE_COLS;
    float vSize = 1.0f / SPRITE_ROWS;

    UVRect uv;
    uv.u0 = col * uSize;
    uv.v0 = row * vSize;
    uv.u1 = uv.u0 + uSize;
    uv.v1 = uv.v0 + vSize;
    return uv;
}

void draw_bullet(GLuint program, GLuint vao, float x, float y, float size, HMM_Mat4 projection)
{
    glUseProgram(program);

    glUniformMatrix4fv(glGetUniformLocation(program, "uProjection"), 1, GL_FALSE, (float*)&projection);
    glUniform2f(glGetUniformLocation(program, "uOffset"), x, y);
    glUniform1f(glGetUniformLocation(program, "uScale"), size * 0.5f);  // uniform scale: width = size, height = size

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
