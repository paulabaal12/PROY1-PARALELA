#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "entities.h"

#define WINDOW_W 800
#define WINDOW_H 600

static SDL_Color hsv_to_rgb(float h, float s, float v){
    float c = v*s;
    float x = c*(1 - fabsf(fmodf(h/60.0f, 2.0f) - 1));
    float m = v - c;
    float r=0,g=0,b=0;
    if (h<60){r=c;g=x;b=0;} else if (h<120){r=x;g=c;b=0;} else if (h<180){r=0;g=c;b=x;} else if(h<240){r=0;g=x;b=c;} else if(h<300){r=x;g=0;b=c;} else {r=c;g=0;b=x;}
    SDL_Color col = { (Uint8)((r+m)*255), (Uint8)((g+m)*255), (Uint8)((b+m)*255), 255};
    return col;
}

static void draw_vertical_gradient(SDL_Renderer *ren, int w, int h, float t){
    // gradiente animado en HSV (hue desplazado con el tiempo)
    for (int y=0; y<h; ++y){
        float p = (float)y/(float)(h-1);
        float hue = fmodf((t*40.0f + p*180.0f), 360.0f);
        SDL_Color c = hsv_to_rgb(hue, 0.6f, 0.25f + 0.65f*p);
        SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, 255);
        SDL_RenderDrawLine(ren, 0, y, w-1, y);
    }
}

int main(int argc, char **argv){
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0){
        fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }
    if (TTF_Init() != 0){
        fprintf(stderr, "TTF_Init error: %s\n", TTF_GetError());
        return 1;
    }

    int N = 200; // por defecto
    if (argc >= 2) N = atoi(argv[1]);
    if (N < 1) N = 1;

    SDL_Window *win = SDL_CreateWindow("Screensaver Notas (Secuencial)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_W, WINDOW_H, SDL_WINDOW_SHOWN);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren){
        // fallback software
        ren = SDL_CreateRenderer(win, -1, 0);
    }

    // Cargar fuente para FPS (usar una fuente del sistema si existe)
    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 14);
    if (!font) {
        fprintf(stderr, "No se pudo abrir fuente TTF, FPS se omitirá: %s\n", TTF_GetError());
    }

    Note *notes = (Note*)malloc(sizeof(Note)*N);
    notes_init(notes, N, WINDOW_W, WINDOW_H, (unsigned)time(NULL));

    Uint64 perf_freq = SDL_GetPerformanceFrequency();
    Uint64 t_prev = SDL_GetPerformanceCounter();
    float t_seconds = 0.0f;

    int frames = 0;
    float acc_time = 0.0f;
    float fps = 0.0f;

    char fps_text[64];
    SDL_Color white = {240,240,240,255};

    int running = 1;
    while (running){
        SDL_Event e;
        while (SDL_PollEvent(&e)){
            if (e.type == SDL_QUIT) running = 0;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) running = 0;
        }

        Uint64 t_now = SDL_GetPerformanceCounter();
        float dt = (float)(t_now - t_prev) / (float)perf_freq;
        if (dt > 0.05f) dt = 0.05f; // clamp por si se pausa
        t_prev = t_now;
        t_seconds += dt;


        for (int i=0; i<N; ++i) {
        note_update_bounce(&notes[i], dt, WINDOW_W, WINDOW_H);
        }
        // Render
        draw_vertical_gradient(ren, WINDOW_W, WINDOW_H, t_seconds);
        for (int i=0;i<N;++i) note_render(ren, &notes[i]);

        // FPS
        frames++; acc_time += dt;
        if (acc_time >= 0.25f){ fps = frames/acc_time; frames=0; acc_time=0.0f; }
        if (font){
            snprintf(fps_text, sizeof(fps_text), "FPS: %.1f  N=%d", fps, N);
            SDL_Surface *surf = TTF_RenderText_Blended(font, fps_text, white);
            SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
            SDL_Rect dst = { 10, 10, surf->w, surf->h };
            SDL_RenderCopy(ren, tex, NULL, &dst);
            SDL_FreeSurface(surf); SDL_DestroyTexture(tex);
        }

        SDL_RenderPresent(ren);
    }

    if (font) TTF_CloseFont(font);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    free(notes);
    return 0;
}