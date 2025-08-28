#include "entities.h"
#include <math.h>
#include <stdlib.h>

static float frand01(void) { return (float)rand() / (float)RAND_MAX; }

// Escala mayor simple (C4..C5)
static const float SCALE_HZ[] = {
    261.63f, 293.66f, 329.63f, 349.23f, 392.00f, 440.00f, 493.88f, 523.25f
};

static Uint8 clampu8(int v){ if(v<0) return 0; if(v>255) return 255; return (Uint8)v; }

void notes_init(Note *notes, int N, int w, int h, unsigned int seed) {
    (void)w; (void)h;
    srand(seed);
    for (int i=0;i<N;++i){
        Note *n = &notes[i];
        n->radius = 10 + 10*frand01();
        n->x = 0; // se calculará dinámicamente
        n->y = 0;
        n->vx = 0;
        n->vy = 0;
        n->r = clampu8(80 + (int)(175*frand01()));
        n->g = clampu8(80 + (int)(175*frand01()));
        n->b = clampu8(80 + (int)(175*frand01()));
        float r = frand01();
        n->kind = (r<0.25f)?NOTE_HALF : (r<0.55f?NOTE_QUARTER : (r<0.85f?NOTE_EIGHTH:NOTE_SIXTEENTH));
        n->pitch_hz = SCALE_HZ[rand()% (int)(sizeof(SCALE_HZ)/sizeof(SCALE_HZ[0]))];
    }
}


// t_seconds: tiempo global, w/h: tamaño ventana, i: índice de la nota, N: total
void note_update_creative(Note *n, int i, int N, float t_seconds, int w, int h) {

    // Fases: cuadrícula -> figura -> dispersión -> cuadrícula siguiente
    int n_figs = 3;
    int fig = ((int)(t_seconds / 7.0f)) % n_figs;
    int next_fig = (fig + 1) % n_figs;
    float t_phase = fmodf(t_seconds, 7.0f) / 7.0f; // 0..1
    float cx = w / 2.0f;
    float cy = h / 2.0f;

    // Posición cuadrícula (pantalla llena)
    int cols = (int)sqrtf((float)N * w / h);
    int rows = (N + cols - 1) / cols;
    int col = i % cols;
    int row = i / cols;
    float xgap = (float)w / (float)(cols+1);
    float ygap = (float)h / (float)(rows+1);
    float grid_x = xgap * (col+1);
    float grid_y = ygap * (row+1);

    // Posición figura actual
    float theta = 2 * 3.1415926f * (float)i / (float)N;
    float fx = grid_x, fy = grid_y;
    if (fig == 0) {
        // Corazón
        float R = (h < w ? h : w) * 0.28f;
        float t = theta;
        fx = cx + R * 16 * powf(sinf(t), 3) / 17.0f;
        fy = cy - R * (13 * cosf(t) - 5 * cosf(2*t) - 2 * cosf(3*t) - cosf(4*t)) / 17.0f;
    } else if (fig == 1) {
        // Flor
        float R = (h < w ? h : w) * 0.36f;
        float petals = 6.0f + 2.0f * sinf(t_seconds*0.5f);
        float r = R * (0.7f + 0.3f * sinf(petals * theta + t_seconds));
        fx = cx + r * cosf(theta);
        fy = cy + r * sinf(theta);
    } else if (fig == 2) {
        // Lemniscata
        float a = (h < w ? h : w) * 0.25f;
        float denom = 1.0f + sinf(theta) * sinf(theta);
        fx = cx + (a * cosf(theta)) / denom;
        fy = cy + (a * sinf(theta) * cosf(theta)) / denom;
    } else {
        // Estrella
        float R = (h < w ? h : w) * 0.45f;
        float golden = 2.399963f; // ángulo áureo en radianes
        float r = R * sqrtf((float)i / (float)N);
        float ang = i * golden + t_seconds*0.2f;
        fx = cx + r * cosf(ang);
        fy = cy + r * sinf(ang);
    }

    // Posición figura siguiente (para la transición de dispersión a la siguiente figura)
    float fx_next = grid_x, fy_next = grid_y;
    if (next_fig == 0) {
        float R = (h < w ? h : w) * 0.28f;
        float t = theta;
        fx_next = cx + R * 16 * powf(sinf(t), 3) / 17.0f;
        fy_next = cy - R * (13 * cosf(t) - 5 * cosf(2*t) - 2 * cosf(3*t) - cosf(4*t)) / 17.0f;
    } else if (next_fig == 1) {
        float R = (h < w ? h : w) * 0.36f;
        float petals = 6.0f + 2.0f * sinf((t_seconds+7.0f)*0.5f);
        float r = R * (0.7f + 0.3f * sinf(petals * theta + t_seconds+7.0f));
        fx_next = cx + r * cosf(theta);
        fy_next = cy + r * sinf(theta);
    } else {
        float R = (h < w ? h : w) * 0.33f;
        int spikes = 5 + (int)(2.0f * (0.5f + 0.5f * sinf((t_seconds+7.0f)*0.6f)));
        float star = 0.65f + 0.35f * cosf(spikes * theta + (t_seconds+7.0f)*1.2f);
        float r = R * star;
        fx_next = cx + r * cosf(theta);
        fy_next = cy + r * sinf(theta);
    }

    // Posición de dispersión (explosión desde el centro)
    float disp_r = (h < w ? h : w) * (0.45f + 0.25f * sinf(t_seconds + i));
    float disp_x = cx + disp_r * cosf(theta + sinf(i));
    float disp_y = cy + disp_r * sinf(theta + cosf(i));

    // Fases: 0-0.6 transición cuadrícula->figura, 0.6-0.8 dispersión, 0.8-1.0 dispersión->siguiente figura
    float interp;
    if (t_phase < 0.6f) {
        interp = 0.5f - 0.5f * cosf((t_phase/0.6f) * 3.1415926f);
        n->x = (1.0f - interp) * grid_x + interp * fx;
        n->y = (1.0f - interp) * grid_y + interp * fy;
    } else if (t_phase < 0.8f) {
        float dphase = (t_phase-0.6f)/0.2f;
        interp = 0.5f - 0.5f * cosf(dphase * 3.1415926f);
        n->x = (1.0f - interp) * fx + interp * disp_x;
        n->y = (1.0f - interp) * fy + interp * disp_y;
    } else {
        float dphase = (t_phase-0.8f)/0.2f;
        interp = 0.5f - 0.5f * cosf(dphase * 3.1415926f);
        n->x = (1.0f - interp) * disp_x + interp * fx_next;
        n->y = (1.0f - interp) * disp_y + interp * fy_next;
    }
    n->radius = 9.0f + 3.0f * sinf(t_seconds + i*0.5f);

    // Color arcoíris animado
    float hue = fmodf((t_seconds * 30.0f + i * 360.0f / N), 360.0f);
    float s = 0.7f, v = 0.85f;
    float C = v * s, X = C * (1 - fabsf(fmodf(hue / 60.0f, 2.0f) - 1)), m = v - C;
    float rr = 0, gg = 0, bb = 0;
    if (hue < 60) { rr = C; gg = X; bb = 0; }
    else if (hue < 120) { rr = X; gg = C; bb = 0; }
    else if (hue < 180) { rr = 0; gg = C; bb = X; }
    else if (hue < 240) { rr = 0; gg = X; bb = C; }
    else if (hue < 300) { rr = X; gg = 0; bb = C; }
    else { rr = C; gg = 0; bb = X; }
    n->r = clampu8((int)((rr + m) * 255));
    n->g = clampu8((int)((gg + m) * 255));
    n->b = clampu8((int)((bb + m) * 255));
}

// Dibujo de círculo relleno básico (sin SDL_gfx)
static void fill_circle(SDL_Renderer *ren, int cx, int cy, int r){
    for (int dy=-r; dy<=r; ++dy){
        int dx = (int)sqrtf((float)(r*r - dy*dy));
        SDL_RenderDrawLine(ren, cx-dx, cy+dy, cx+dx, cy+dy);
    }
}

void note_render(SDL_Renderer *ren, const Note *n){
    SDL_SetRenderDrawColor(ren, n->r, n->g, n->b, 255);
    // cabeza
    fill_circle(ren, (int)n->x, (int)n->y, (int)n->radius);

    // plica (hacia arriba)
    int stem_h = (int)(n->radius*3.2f);
    int stem_w = (int)fmaxf(2.0f, n->radius*0.3f);
    SDL_Rect stem = { (int)(n->x + n->radius*0.8f), (int)(n->y - stem_h), stem_w, stem_h };
    SDL_RenderFillRect(ren, &stem);

    // banderas (según tipo)
    int flags = (n->kind==NOTE_EIGHTH)?1 : (n->kind==NOTE_SIXTEENTH?2:0);
    for (int i=0;i<flags;++i){
        int y0 = stem.y + 6 + i*10;
        SDL_RenderDrawLine(ren, stem.x + stem.w, y0, stem.x + stem.w + (int)(n->radius*1.4f), y0 + 6);
        SDL_RenderDrawLine(ren, stem.x + stem.w, y0+1, stem.x + stem.w + (int)(n->radius*1.4f), y0 + 7);
    }
}