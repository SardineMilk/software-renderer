#include <SDL2/SDL.h>
#include <stdio.h>


typedef struct {
    int r;
    int g;
    int b;
} Color;

typedef struct {
    int width;
    int height;
} Uniforms;

static Color shader(int x, int y, const Uniforms *uni) {
    int r = (float)x / ((float)uni->width/255.0);
    int g = 0;
    int b = (float)y / ((float)uni->height/255.0);

    Color color = {r, g, b};
    return color;
}

int main() {
    // Create window
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    int width = 800;
    int height = 600;
    SDL_Window *win = SDL_CreateWindow(
        "SDL2 Software Renderer", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height, SDL_WINDOW_SHOWN
    );

    if (!win) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create surface the same size as the window
    SDL_Surface *surf = SDL_GetWindowSurface(win);
    if (!surf) {
        printf("SDL_Surface Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    Uniforms uni = {width, height};

    // Main Loop
    int running = 1;
    SDL_Event event;
    while (running) {
        // Event handling
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;
        }

        // Lock surface pixels for writing
        SDL_LockSurface(surf);
        uint32_t *pixels = (uint32_t *)surf->pixels;
        int pitch = surf->pitch / 4;  // Number of bytes between the start of each row in memory

        // Loop over every pixel
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                Color pixel_color = shader(x, y, &uni);
                pixels[(y * pitch) + x] = 0xFF000000 | (pixel_color.r << 16) | (pixel_color.g << 8) | pixel_color.b;
            }
        }

        SDL_UnlockSurface(surf);
        SDL_UpdateWindowSurface(win);
        SDL_Delay(16);
    }

    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
