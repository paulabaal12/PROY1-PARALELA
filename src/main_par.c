#include <omp.h>
#include <string.h>
#include <omp.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "entities.h"

#define WINDOW_W 800
#define WINDOW_H 600

static void pastel_gradient(SDL_Renderer *ren, int w, int h, float t) {
    SDL_Color stops[5] = {
        {180, 220, 255, 255}, // celeste
        {220, 190, 255, 255}, // lila
        {255, 210, 230, 255}, // rosa
        {255, 245, 200, 255}, // amarillo
        {200, 255, 220, 255}  // menta
    };
    int nstops = 5;
    float anim = t * 0.12f; // desplazamiento lineal suave
    for (int y=0; y<h; ++y) {
        float p = (float)y/(float)(h-1);
        float pos = fmodf(p * nstops + anim, nstops);
        if (pos < 0) pos += nstops;
        int idx = (int)pos;
        float frac = pos - idx;
        SDL_Color c1 = stops[idx % nstops];
        SDL_Color c2 = stops[(idx+1) % nstops];
        Uint8 r = (Uint8)((1-frac)*c1.r + frac*c2.r);
        Uint8 g = (Uint8)((1-frac)*c1.g + frac*c2.g);
        Uint8 b = (Uint8)((1-frac)*c1.b + frac*c2.b);
        SDL_SetRenderDrawColor(ren, r, g, b, 255);
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


    const char *mode_str = "Paralelo";
    printf("Modo de ejecución: %s\n", mode_str);

    char window_title[128];
    snprintf(window_title, sizeof(window_title), "Screensaver Notas (%s)", mode_str);
    SDL_Window *win = SDL_CreateWindow(window_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_W, WINDOW_H, SDL_WINDOW_SHOWN);
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

    #define SPEEDUP_HISTORY 64
    double par_times[SPEEDUP_HISTORY] = {0};
    int par_index = 0;

    int running = 1;
    float fps_sum = 0.0f;
    int fps_count = 0;

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

        // Medición de tiempo para speedup

        // --- Modo Paralelo ---
        double t0 = omp_get_wtime();
        #pragma omp parallel for
        for (int i=0; i<N; ++i) {
            note_update_creative(&notes[i], i, N, t_seconds, WINDOW_W, WINDOW_H);
        }
        double t1 = omp_get_wtime();
        double elapsed = t1-t0;
        par_times[par_index % SPEEDUP_HISTORY] = elapsed;
        par_index++;
       

        // Render
        pastel_gradient(ren, WINDOW_W, WINDOW_H, t_seconds);
        for (int i=0;i<N;++i) note_render(ren, &notes[i]);

        // FPS
        frames++; acc_time += dt;
        if (acc_time >= 0.25f){
            fps = frames/acc_time;
            frames=0; acc_time=0.0f;
            fps_sum += fps;
            fps_count++;
        }
        if (font){
            snprintf(fps_text, sizeof(fps_text), "FPS: %.1f  N=%d  Modo: %s", fps, N, mode_str);
            SDL_Surface *surf = TTF_RenderText_Blended(font, fps_text, white);
            SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
            SDL_Rect dst = { 10, 10, surf->w, surf->h };
            SDL_RenderCopy(ren, tex, NULL, &dst);
            SDL_FreeSurface(surf); SDL_DestroyTexture(tex);
        }

        SDL_RenderPresent(ren);
    }
    
    // === Estadísticas Finales (PARALELO) ===
    printf("\n=== Estadísticas Finales (Paralelo) ===\n");
    int npar = par_index < SPEEDUP_HISTORY ? par_index : SPEEDUP_HISTORY;
    double sum_par = 0;
    for (int i = 0; i < npar; ++i) sum_par += par_times[i];
    double avg_par = npar ? sum_par / npar : 0.0;

    if (npar > 0)
        printf("Tiempo promedio paralelo: %.6f segundos\n", avg_par);
    else
        printf("Tiempo promedio paralelo: N/A (no se midió)\n");

    float avg_fps = (fps_count > 0) ? fps_sum / fps_count : 0.0f;
    printf("FPS promedio: %.2f\n", avg_fps);
    printf("Mediciones paralelas: %d\n", npar);

    fflush(stdout);

    if (font) TTF_CloseFont(font);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    free(notes);
    return 0;

}