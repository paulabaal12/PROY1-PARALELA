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
    (void)t;
    SDL_Color stops[5] = {
        {180, 220, 255, 255}, // celeste
        {220, 190, 255, 255}, // lila
        {255, 210, 230, 255}, // rosa
        {255, 245, 200, 255}, // amarillo
        {200, 255, 220, 255}  // menta
    };
    int nstops = 5;
    for (int y=0; y<h; ++y) {
        float p = (float)y/(float)(h-1);
        float pos = p * (nstops-1);
        int idx = (int)pos;
        float frac = pos - idx;
        SDL_Color c1 = stops[idx];
        SDL_Color c2 = stops[(idx+1<nstops)?idx+1:idx];
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

    // Modo de ejecución: 0=secuencial, 1=paralelo, 2=aleatorio
    int mode = 0;
    if (argc >= 3) {
        if (strcmp(argv[2], "paralelo") == 0 || strcmp(argv[2], "parallel") == 0)
            mode = 1;
        else if (strcmp(argv[2], "aleatorio") == 0 || strcmp(argv[2], "random") == 0)
            mode = 2;
        else
            mode = 0;
    }
    const char *mode_str = (mode == 0) ? "Secuencial" : (mode == 1) ? "Paralelo" : "Aleatorio";
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
    double seq_times[SPEEDUP_HISTORY] = {0};
    int par_index = 0, seq_index = 0;

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

        // Medición de tiempo para speedup
        if (mode == 0) {
            // --- Modo Secuencial ---
            double t0 = omp_get_wtime();
            for (int i=0; i<N; ++i) {
                note_update_creative(&notes[i], i, N, t_seconds, WINDOW_W, WINDOW_H);
            }
            double t1 = omp_get_wtime();
            double elapsed = t1-t0;
            printf("[Speedup] Tiempo secuencial: %f segundos\n", elapsed);
            seq_times[seq_index % SPEEDUP_HISTORY] = elapsed;
            seq_index++;
        } else if (mode == 1) {
            // --- Modo Paralelo ---
            double t0 = omp_get_wtime();
            #pragma omp parallel for
            for (int i=0; i<N; ++i) {
                note_update_creative(&notes[i], i, N, t_seconds, WINDOW_W, WINDOW_H);
            }
            double t1 = omp_get_wtime();
            double elapsed = t1-t0;
            printf("[Speedup] Tiempo paralelo: %f segundos\n", elapsed);
            par_times[par_index % SPEEDUP_HISTORY] = elapsed;
            par_index++;
        } else {
            // --- Modo Aleatorio: mide ambos tiempos en cada frame ---
            // Secuencial
            double t0 = omp_get_wtime();
            for (int i=0; i<N; ++i) {
                note_update_creative(&notes[i], i, N, t_seconds, WINDOW_W, WINDOW_H);
            }
            double t1 = omp_get_wtime();
            double elapsed_seq = t1-t0;
            seq_times[seq_index % SPEEDUP_HISTORY] = elapsed_seq;
            seq_index++;
            // Paralelo
            t0 = omp_get_wtime();
            #pragma omp parallel for
            for (int i=0; i<N; ++i) {
                note_update_creative(&notes[i], i, N, t_seconds, WINDOW_W, WINDOW_H);
            }
            t1 = omp_get_wtime();
            double elapsed_par = t1-t0;
            par_times[par_index % SPEEDUP_HISTORY] = elapsed_par;
            par_index++;
            printf("[Speedup] Tiempo secuencial: %f s | Tiempo paralelo: %f s\n", elapsed_seq, elapsed_par);
        }

        // Render
        pastel_gradient(ren, WINDOW_W, WINDOW_H, t_seconds);
        for (int i=0;i<N;++i) note_render(ren, &notes[i]);

        // FPS
        frames++; acc_time += dt;
        if (acc_time >= 0.25f){ fps = frames/acc_time; frames=0; acc_time=0.0f; }
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
    // Estadísticas de tiempos
    printf("\n=== Estadísticas Finales ===\n");
    int npar = par_index < SPEEDUP_HISTORY ? par_index : SPEEDUP_HISTORY;
    int nseq = seq_index < SPEEDUP_HISTORY ? seq_index : SPEEDUP_HISTORY;
    double sum_par = 0, sum_seq = 0;
    for (int i = 0; i < npar; ++i) sum_par += par_times[i];
    for (int i = 0; i < nseq; ++i) sum_seq += seq_times[i];
    double avg_par = npar ? sum_par / npar : 0.0;
    double avg_seq = nseq ? sum_seq / nseq : 0.0;
    double speedup = (avg_par > 0 && avg_seq > 0) ? avg_seq / avg_par : 0.0;
    double efficiency = (speedup > 0) ? speedup / omp_get_max_threads() * 100.0 : 0.0;

    if (nseq > 0)
        printf("Tiempo promedio secuencial: %.6f segundos\n", avg_seq);
    else
        printf("Tiempo promedio secuencial: N/A (no se midió en este modo)\n");

    if (npar > 0)
        printf("Tiempo promedio paralelo: %.6f segundos\n", avg_par);
    else
        printf("Tiempo promedio paralelo: N/A (no se midió en este modo)\n");

    if (npar > 0 && nseq > 0) {
        printf("Speedup: %.2fx\n", speedup);
        printf("Eficiencia: %.1f%%\n", efficiency);
    } else {
        printf("Speedup: N/A (se requieren ambos modos)\n");
        printf("Eficiencia: N/A (se requieren ambos modos)\n");
    }
    printf("Mediciones paralelas: %d\n", npar);
    printf("Mediciones secuenciales: %d\n", nseq);
    if (npar == 0)
        printf("\nNota: No hay tiempos paralelos porque solo ejecutaste el modo secuencial. Ejecuta en modo paralelo para comparar.\n");
    if (nseq == 0)
        printf("\nNota: No hay tiempos secuenciales porque solo ejecutaste el modo paralelo. Ejecuta en modo secuencial para comparar.\n");
    fflush(stdout);

    if (font) TTF_CloseFont(font);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    free(notes);
    return 0;
}