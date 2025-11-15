#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <pthread.h>

#include "vector.h"

// Size of window
#define WINDOW_WIDTH 720
#define WINDOW_HEIGHT 720

#define RENDER_WIDTH 180 
#define RENDER_HEIGHT 180

// Hardcoded delay between frames
#define FRAME_DELAY 16

// Camera variables
#define MOVE_SPEED 0.03f
#define ROT_SPEED  0.01f

#define M_PI_2 1.57079632679f

typedef struct {
    int r;
    int g;
    int b;
} Color;

typedef struct {
    // 1: IsLeaf
    // 31: ChildPtr
    uint32_t ChildPtr;
    uint64_t ChildMask;
} Chunk;


typedef struct {
    float m[3][3];
} Mat3;

typedef struct {
    Vec3 position;
    bool hit;
} HitInfo;

typedef struct {
    Vec3 position;
    Vec3 rotation;
} Camera;


typedef struct {
    int y_start;
    int y_end;
    Camera *camera;
    Vec3 forward, right, up;
    uint32_t *pixels;
    int pitch;
} RenderJob;

static Mat3 mat3_mul(Mat3 a, Mat3 b) {
    Mat3 r;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            r.m[i][j] = a.m[i][0]*b.m[0][j] + a.m[i][1]*b.m[1][j] + a.m[i][2]*b.m[2][j];
    return r;
}

static Vec3 mat3_mul_vec3(Mat3 m, Vec3 v) {
    return (Vec3){
        m.m[0][0]*v.x + m.m[0][1]*v.y + m.m[0][2]*v.z,
        m.m[1][0]*v.x + m.m[1][1]*v.y + m.m[1][2]*v.z,
        m.m[2][0]*v.x + m.m[2][1]*v.y + m.m[2][2]*v.z
    };
}

Vec3 normalize(Vec3 v) {
    float len = length(v);
    if (len == 0) return v;
    return scale(v, 1.0f / len);
}

static HitInfo raycast(Vec3 ro, Vec3 rd) {
    Vec3 ray_position = ro;
    bool hit = false;
    Vec3 intersection_position = ray_position;
    Vec3 inv_rd = (Vec3){1.0f/rd.x, 1.0f/rd.y, 1.0f/rd.z};

    // Cube bounds
    const float xmin = 0.0f, ymin = 0.0f, zmin = 0.0f;
    const float xmax = 1.0f, ymax = 1.0f, zmax = 1.0f;

    // Compute intersection using slab method
    float t1 = (xmin - ro.x) * inv_rd.x;
    float t2 = (xmax - ro.x) * inv_rd.x;
    float tmin = fminf(t1, t2);
    float tmax = fmaxf(t1, t2);

    float ty1 = (ymin - ro.y) * inv_rd.y;
    float ty2 = (ymax - ro.y) * inv_rd.y;
    tmin = fmaxf(tmin, fminf(ty1, ty2));
    tmax = fminf(tmax, fmaxf(ty1, ty2));

    float tz1 = (zmin - ro.z) * inv_rd.z;
    float tz2 = (zmax - ro.z) * inv_rd.z;
    tmin = fmaxf(tmin, fminf(tz1, tz2));
    tmax = fminf(tmax, fmaxf(tz1, tz2));

    if (tmax >= tmin && tmax >= 0.0f) {
        hit = true;
        float t = (tmin >= 0.0f) ? tmin : tmax;  // pick nearest positive
        intersection_position = (Vec3){
            ro.x + rd.x * t,
            ro.y + rd.y * t,
            ro.z + rd.z * t
        };
    }

    return (HitInfo){intersection_position, hit};
}

static Color shader(int x, int y, Camera *camera, Vec3 forward, Vec3 right, Vec3 up) {
    float u = (2.0f * x / RENDER_WIDTH - 1.0f);
    float v = (2.0f * y / RENDER_HEIGHT - 1.0f);

    // direction is forward + a small right/up bias depending on the pixel position
    Vec3 ray_direction = add(forward, add(scale(right, u), scale(up, v)));
    ray_direction = normalize(ray_direction);

    Vec3 ray_origin = camera->position;

    HitInfo ray_hit = raycast(ray_origin, ray_direction);


    if (ray_hit.hit) {
        float shade = length(scale(sub(ray_hit.position, (Vec3){0.5, 0.5, 0.5}), 0.5f));
        Color color = {127-(255*shade), 0, 255*shade};
        return color;
    }
    else {
        return (Color){13, 13, 27};
    }
}

static Mat3 camera_matrix(Camera *cam) {
    float pitch = cam->rotation.x;
    float yaw   = cam->rotation.y;

    Mat3 Rx = {{
        {1,          0,           0},
        {0,  cosf(pitch), -sinf(pitch)},
        {0,  sinf(pitch),  cosf(pitch)}
    }};

    Mat3 Ry = {{
        { cosf(yaw), 0, sinf(yaw)},
        {        0,  1,        0},
        {-sinf(yaw), 0, cosf(yaw)}
    }};

    return mat3_mul(Ry, Rx);
}

void* render_thread(void *arg) {
    // Render a chunk of pixels
    RenderJob *job = (RenderJob*)arg;

    for (int y = job->y_start; y < job->y_end; ++y) {
        for (int x = 0; x < RENDER_WIDTH; ++x) {
            Color c = shader(x, y, job->camera, job->forward, job->right, job->up);
            job->pixels[y * job->pitch + x] =
                0xFF000000 | (c.r << 16) | (c.g << 8) | c.b;
        }
    }

    return NULL;
}

int main() {
    // Create window
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow(
        "SDL2 Software Renderer", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN
    );

    if (!win) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create surface the same size as the window
    SDL_Surface *window_surf = SDL_GetWindowSurface(win);
    if (!window_surf) {
        printf("SDL_Surface Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Surface *render_surf = SDL_CreateRGBSurface(
        0, RENDER_WIDTH, RENDER_HEIGHT, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000
    );
    if (!render_surf) {
        printf("Render surface error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Camera
    Camera camera = {(Vec3){0, 0, 0}, (Vec3){0, 0, 0}};

    // Multithreading variables
    int thread_count = SDL_GetCPUCount();
    pthread_t threads[16];
    RenderJob jobs[16];

    int rows_per_thread = RENDER_HEIGHT / thread_count;

    // Main Loop
    int running = 1;
    SDL_Event event;
    while (running) {
        Uint64 start = SDL_GetPerformanceCounter();

        // Event handling
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;
            if (event.type == SDL_MOUSEMOTION) {
                int mx = event.motion.xrel; // relative motion since last event
                int my = event.motion.yrel;

                camera.rotation.y += mx * ROT_SPEED; // yaw
                camera.rotation.x -= my * ROT_SPEED; // pitch

                // Clamp pitch
                if (camera.rotation.x > M_PI_2) camera.rotation.x = M_PI_2;
                if (camera.rotation.x < -M_PI_2) camera.rotation.x = -M_PI_2;
            }
        }

        // Keyboard input
        const Uint8 *keys = SDL_GetKeyboardState(NULL);

        // Forward/right movement based on yaw
        Mat3 camMat = camera_matrix(&camera);

        Vec3 forward = mat3_mul_vec3(camMat, (Vec3){0, 0, 1});
        Vec3 right   = mat3_mul_vec3(camMat, (Vec3){1, 0, 0});
        Vec3 up      = mat3_mul_vec3(camMat, (Vec3){0, 1, 0});

        // Movement
        float yaw = camera.rotation.y;
        Vec3 flatForward = { sinf(yaw), 0, cosf(yaw) };
        Vec3 flatRight   = { cosf(yaw), 0,-sinf(yaw) };

        if (keys[SDL_SCANCODE_W]) camera.position = add(camera.position, scale(flatForward, MOVE_SPEED));
        if (keys[SDL_SCANCODE_S]) camera.position = add(camera.position, scale(flatForward,-MOVE_SPEED));
        if (keys[SDL_SCANCODE_A]) camera.position = add(camera.position, scale(flatRight,  -MOVE_SPEED));
        if (keys[SDL_SCANCODE_D]) camera.position = add(camera.position, scale(flatRight,   MOVE_SPEED));
        if (keys[SDL_SCANCODE_SPACE]) camera.position.y -= MOVE_SPEED;
        if (keys[SDL_SCANCODE_LSHIFT]) camera.position.y += MOVE_SPEED;


        uint32_t *pixels = (uint32_t *)render_surf->pixels;
        int pitch = render_surf->pitch / 4;  // Number of bytes between the start of each row in memory


        for (int i = 0; i < thread_count; i++) {
            int y0 = i * rows_per_thread;  // starting y
            int y1 = (i == thread_count - 1) ? RENDER_HEIGHT : y0 + rows_per_thread;  // ending y. if last thread, take all the rest

            jobs[i] = (RenderJob){
                .y_start = y0,
                .y_end   = y1,
                .camera  = &camera,
                .forward = forward,
                .right   = right,
                .up      = up,
                .pixels  = pixels,
                .pitch   = pitch
            };

            pthread_create(&threads[i], NULL, render_thread, &jobs[i]);
        }

        for (int i = 0; i < thread_count; i++)
            pthread_join(threads[i], NULL);
        /*
        // Loop over every pixel
        for (int y = 0; y < RENDER_HEIGHT; ++y) {
            for (int x = 0; x < RENDER_WIDTH; ++x) {
                Color pixel_color = shader(x, y, &camera, forward, right, up);
                pixels[(y * pitch) + x] = 0xFF000000 | (pixel_color.r << 16) | (pixel_color.g << 8) | pixel_color.b;
            }
        }
        */
        SDL_BlitScaled(render_surf, NULL, window_surf, NULL);   
        SDL_UpdateWindowSurface(win);

        Uint64 end = SDL_GetPerformanceCounter();
        float elapsed = (float)(end - start) / (float)SDL_GetPerformanceFrequency();
        printf("Uncapped FPS: %f\n", 1.0f / elapsed);
        
        float target = FRAME_DELAY / 1000.0f;
        if (elapsed < target)
            SDL_Delay((Uint32)((target - elapsed) * 1000.0f));
    }

    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
