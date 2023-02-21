#pragma once

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer.h"

#include "SDL.h"

#include "hv2/hv2.hpp"

extern char _binary_debug_res_UbuntuMono_Regular_ttf_start[];
extern char _binary_debug_res_UbuntuMono_Regular_ttf_end[];

const char* font_start = _binary_debug_res_UbuntuMono_Regular_ttf_start;
const char* font_end = _binary_debug_res_UbuntuMono_Regular_ttf_end;
const size_t font_size = font_end - font_start;

struct hv2_debug_t {
    hv2_t* cpu;

    SDL_Window* window;
    SDL_Renderer* renderer;

    ImFont* font;
    ImGuiIO* io;

    bool open;
};

hv2_debug_t* hv2_debug_create() {
    return new hv2_debug_t;
}

void hv2_debug_init(hv2_debug_t* debug, hv2_t* cpu) {
    debug->cpu = cpu;

    debug->window = SDL_CreateWindow(
        "HV2 debugger",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        800, 600,
        0
    );

    debug->renderer = SDL_CreateRenderer(
        debug->window,
        -1,
        SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED
    );

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    
    debug->io = &ImGui::GetIO();

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForSDLRenderer(debug->window, debug->renderer);
    ImGui_ImplSDLRenderer_Init(debug->renderer);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    debug->io->Fonts->AddFontDefault();

    debug->io->IniFilename = NULL;

    debug->font = debug->io->Fonts->AddFontFromMemoryTTF((void*)font_start, font_size, 20.0);

    debug->open = true;
}

void hv2_debug_close(hv2_debug_t* debug) {
    debug->open = false;

    ImGui_ImplSDLRenderer_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(debug->renderer);
    SDL_DestroyWindow(debug->window);
    SDL_Quit();
}

bool hv2_debug_is_open(hv2_debug_t* debug) {
    return debug->open;
}

using namespace ImGui;

void hv2_debug_update(hv2_debug_t* debug) {
    if (!debug->open)
        return;
    
    ImGui_ImplSDLRenderer_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    
    NewFrame();

    PushFont(debug->font);

    ShowDemoWindow();

    SetNextWindowSize(ImVec2(800, 600));
    SetNextWindowPos(ImVec2(0, 0));
    Begin("CPU", nullptr,
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove
    );

    End();

    PopFont();

    EndFrame();

    Render();
    SDL_RenderSetScale(debug->renderer, debug->io->DisplayFramebufferScale.x, debug->io->DisplayFramebufferScale.y);
    SDL_RenderClear(debug->renderer);
    ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
    SDL_RenderPresent(debug->renderer);

    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);

        switch (event.type) {
            case SDL_QUIT: {
                hv2_debug_close(debug);
            } break;

            case SDL_WINDOWEVENT: {
                if (event.window.event != SDL_WINDOWEVENT_CLOSE)
                    break;
                
                if (event.window.windowID == SDL_GetWindowID(debug->window)) {
                    hv2_debug_close(debug);
                }
            } break;
        }
    }
}

