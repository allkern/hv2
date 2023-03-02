#pragma once

#include "SDL.h"

#include <string>

struct screen_t {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;

    bool open;

    int width, height;
};

screen_t* screen_create() {
    return new screen_t;
}

void screen_destroy(screen_t* screen) {
    SDL_DestroyTexture(screen->texture);
    SDL_DestroyRenderer(screen->renderer);
    SDL_DestroyWindow(screen->window);

    SDL_Quit();

    delete screen;
}

void screen_init(screen_t* screen, std::string title, int width, int height, int scale = 1, bool fullscreen = false) {
    screen->width = width;
    screen->height = height;

    screen->window = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        width * scale, height * scale,
        SDL_WINDOW_OPENGL | (fullscreen ? SDL_WINDOW_FULLSCREEN : 0)
    );

    screen->renderer = SDL_CreateRenderer(
        screen->window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    screen->texture = SDL_CreateTexture(
        screen->renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        width, height
    );

    screen->open = true;
}

void screen_update(screen_t* screen, uint32_t* buf) {
    SDL_UpdateTexture(screen->texture, NULL, buf, screen->width * sizeof(uint32_t));
    SDL_RenderCopy(screen->renderer, screen->texture, NULL, NULL);
    SDL_RenderPresent(screen->renderer);

    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT: {
                screen->open = false;
            } break;
        }
    }
}