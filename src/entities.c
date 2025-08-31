#include <math.h>
#include <stdlib.h>

void figure_heart(float *x, float *y, float cx, float cy, float theta, float scale) {
    float R = scale;
    float t = theta;
    *x = cx + R * 16 * powf(sinf(t), 3) / 17.0f;
    *y = cy - R * (13 * cosf(t) - 5 * cosf(2*t) - 2 * cosf(3*t) - cosf(4*t)) / 17.0f;
}

// Flor animada
void figure_flower(float *x, float *y, float cx, float cy, float theta, float scale, float t_seconds) {
    float R = scale;
    float petals = 6.0f + 2.0f * sinf(t_seconds*0.5f);
    float r = R * (0.7f + 0.3f * sinf(petals * theta + t_seconds));
    *x = cx + r * cosf(theta);
    *y = cy + r * sinf(theta);
}

// Lemniscata (infinito)
void figure_lemniscate(float *x, float *y, float cx, float cy, float theta, float scale, float anim) {
    float a = scale;
    float theta_anim = theta * anim;
    float denom = 1.0f + sinf(theta_anim) * sinf(theta_anim);
    *x = cx + (a * cosf(theta_anim)) / denom;
    *y = cy + (a * sinf(theta_anim) * cosf(theta_anim)) / denom;
}

// Espiral
void figure_spiral(float *x, float *y, float cx, float cy, float theta, float scale, float t_seconds) {
    // Espiral que inicia en el centro y se expande hacia afuera
    float vueltas = 2.5f + 1.5f * sinf(t_seconds * 0.2f); // número de vueltas animado
    float t_norm = theta / (2.0f * 3.1415926f); // 0..1
    float r = scale * t_norm * vueltas;
    float angle = theta + t_seconds * 0.7f;
    *x = cx + r * cosf(angle);
    *y = cy + r * sinf(angle);
}

// Estrella pulsante
void figure_star(float *x, float *y, float cx, float cy, float theta, float scale, float t_seconds) {
    float R = scale;
    int spikes = 5 + (int)(2.0f * (0.5f + 0.5f * sinf(t_seconds*0.6f)));
    float star = 0.65f + 0.35f * cosf(spikes * theta + t_seconds*1.2f);
    float r = R * star * (1.0f + 0.1f * sinf(t_seconds*2.0f));
    *x = cx + r * cosf(theta);
    *y = cy + r * sinf(theta);
}

// Onda senoidal
void figure_wave(float *x, float *y, float cx, float cy, float theta, float scale, float t_seconds, int i, int N) {
    float R = scale;
    float x0 = cx + (theta - 3.1415926f) * R;
    float y0 = cy + sinf(theta * 2 + t_seconds * 2.0f) * R * 0.4f;
    *x = x0;
    *y = y0;
    (void)i; (void)N; // Mark unused
}

// Puedes agregar más figuras aquí...

#include "entities.h"

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
    // Burbujas físicas (colisiones y rebotes)
    // Para la versión paralela, el cálculo de posiciones y colisiones puede hacerse con OpenMP
    void figure_bubbles(Note *notes, int N, int i, int w, int h, float t_seconds) {
        Note *n = &notes[i];
        // Inicialización de velocidad y posición si es la primera vez
        if (n->vx == 0 && n->vy == 0) {
            n->vx = 80.0f * (frand01() - 0.5f);
            n->vy = 80.0f * (frand01() - 0.5f);
            n->x = w * frand01();
            n->y = h * frand01();
        }
        // Movimiento
        n->x += n->vx * 0.016f; // Suponiendo ~60 FPS
        n->y += n->vy * 0.016f;
        // Rebote con bordes
        if (n->x < n->radius) { n->x = n->radius; n->vx *= -1; }
        if (n->x > w-n->radius) { n->x = w-n->radius; n->vx *= -1; }
        if (n->y < n->radius) { n->y = n->radius; n->vy *= -1; }
        if (n->y > h-n->radius) { n->y = h-n->radius; n->vy *= -1; }
        // Colisiones simples entre burbujas (solo repulsión)
        for (int j=0; j<N; ++j) {
            if (i==j) continue;
            Note *m = &notes[j];
            float dx = n->x - m->x;
            float dy = n->y - m->y;
            float dist = sqrtf(dx*dx + dy*dy);
            float min_dist = n->radius + m->radius;
            if (dist < min_dist && dist > 1e-2f) {
                float overlap = 0.5f * (min_dist - dist);
                n->x += (dx/dist) * overlap;
                n->y += (dy/dist) * overlap;
                n->vx += (dx/dist) * 8.0f;
                n->vy += (dy/dist) * 8.0f;
            }
        }
        // Efecto de tamaño pulsante
        n->radius = 12.0f + 4.0f * sinf(t_seconds*2.0f + i);
    }


// t_seconds: tiempo global, w/h: tamaño ventana, i: índice de la nota, N: total
void note_update_creative(Note *n, int i, int N, float t_seconds, int w, int h) {

    float cx = w / 2.0f;
    float cy = h / 2.0f;
    int cols = (int)sqrtf((float)N * w / h);
    int rows = (N + cols - 1) / cols;
    int col = i % cols;
    int row = i / cols;
    float xgap = (float)w / (float)(cols+1);
    float ygap = (float)h / (float)(rows+1);
    float grid_x = xgap * (col+1);
    float grid_y = ygap * (row+1);

    // Variables for figure selection and phase
    int fig = 0, next_fig = 1;
    float t_phase = 0.0f;
    const int n_figs = 7;
    float fig_duration = 7.0f;
    if (t_seconds >= 7.0f) {
        int phase_idx = (int)((t_seconds - 7.0f) / fig_duration);
        fig = phase_idx % n_figs;
        next_fig = (fig + 1) % n_figs;
        t_phase = fmodf((t_seconds - 7.0f), fig_duration) / fig_duration;
    }

    // --- Fase inicial: cuadrícula y transición creativa al corazón ---
    if (t_seconds < 4.0f) {
        n->x = grid_x;
        n->y = grid_y;
        n->radius = 10.0f + 2.0f * sinf(t_seconds + i*0.5f);
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
        return;
    } else if (t_seconds < 7.0f) {
        // Transición cuadrícula -> dispersión -> puntos aleatorios -> corazón
        float t_local = t_seconds - 4.0f;
        float theta = 2 * 3.1415926f * (float)i / (float)N;
        float fx, fy, disp_x, disp_y, rand_x, rand_y;
        figure_heart(&fx, &fy, cx, cy, theta, (h < w ? h : w) * 0.28f);
        float disp_r = (h < w ? h : w) * (0.45f + 0.25f * sinf(t_seconds + i));
        disp_x = cx + disp_r * cosf(theta + sinf(i));
        disp_y = cy + disp_r * sinf(theta + cosf(i));
        rand_x = w * (0.1f + 0.8f * ((float)((i*73)%N)/(float)N));
        rand_y = h * (0.1f + 0.8f * ((float)((i*97)%N)/(float)N));
        float t_phase_local = t_local / 3.0f;
        float interp = 0.0f;
        if (t_phase_local < 0.33f) {
            interp = 0.5f - 0.5f * cosf((t_phase_local/0.33f) * 3.1415926f);
            n->x = (1.0f - interp) * grid_x + interp * disp_x;
            n->y = (1.0f - interp) * grid_y + interp * disp_y;
        } else if (t_phase_local < 0.66f) {
            float dphase = (t_phase_local-0.33f)/0.33f;
            interp = 0.5f - 0.5f * cosf(dphase * 3.1415926f);
            n->x = (1.0f - interp) * disp_x + interp * rand_x;
            n->y = (1.0f - interp) * disp_y + interp * rand_y;
        } else {
            float dphase = (t_phase_local-0.66f)/0.34f;
            interp = 0.5f - 0.5f * cosf(dphase * 3.1415926f);
            n->x = (1.0f - interp) * rand_x + interp * fx;
            n->y = (1.0f - interp) * rand_y + interp * fy;
        }
        n->radius = 9.0f + 3.0f * sinf(t_seconds + i*0.5f);
        return;
    }

    // --- Fase principal: transición creativa entre figuras ---
    // Definir theta y escala base
    float theta = 2 * 3.1415926f * (float)i / (float)N;
    float scale = (h < w ? h : w) * 0.28f;
    float fx = 0, fy = 0, fx_next = 0, fy_next = 0;

    // Llamar a la figura actual y la siguiente con su propio efecto
    switch (fig) {
        case 0: // Corazón
            figure_heart(&fx, &fy, cx, cy, theta, scale);
            break;
        case 1: // Flor animada
            figure_flower(&fx, &fy, cx, cy, theta, scale * 1.3f, t_seconds);
            break;
        case 2: // Lemniscata
            figure_lemniscate(&fx, &fy, cx, cy, theta, scale, t_phase < 0.33f ? t_phase/0.33f : 1.0f);
            break;
        case 3: // Espiral
            figure_spiral(&fx, &fy, cx, cy, theta, scale, t_seconds);
            break;
        case 4: // Estrella pulsante
            figure_star(&fx, &fy, cx, cy, theta, scale * 1.2f, t_seconds);
            break;
        case 5: // Onda senoidal
            figure_wave(&fx, &fy, cx, cy, theta, scale * 1.5f, t_seconds, i, N);
            break;
        case 6: // Burbujas físicas
            figure_bubbles((Note*)n - i, N, i, w, h, t_seconds);
            return;
        default:
            fx = grid_x;
            fy = grid_y;
    }
    switch (next_fig) {
        case 0:
            figure_heart(&fx_next, &fy_next, cx, cy, theta, scale);
            break;
        case 1:
            figure_flower(&fx_next, &fy_next, cx, cy, theta, scale * 1.3f, t_seconds+7.0f);
            break;
        case 2:
            figure_lemniscate(&fx_next, &fy_next, cx, cy, theta, scale, 1.0f);
            break;
        case 3:
            figure_spiral(&fx_next, &fy_next, cx, cy, theta, scale, t_seconds+7.0f);
            break;
        case 4:
            figure_star(&fx_next, &fy_next, cx, cy, theta, scale * 1.2f, t_seconds+7.0f);
            break;
        case 5:
            figure_wave(&fx_next, &fy_next, cx, cy, theta, scale * 1.5f, t_seconds+7.0f, i, N);
            break;
        default:
            fx_next = grid_x;
            fy_next = grid_y;
    }

    // Dispersión y puntos aleatorios para la transición
    float disp_r = (h < w ? h : w) * (0.45f + 0.25f * sinf(t_seconds + i));
    float disp_x = cx + disp_r * cosf(theta + sinf(i));
    float disp_y = cy + disp_r * sinf(theta + cosf(i));
    float rand_x = w * (0.1f + 0.8f * ((float)((i*73)%N)/(float)N));
    float rand_y = h * (0.1f + 0.8f * ((float)((i*97)%N)/(float)N));

    // Fases: 0-0.33 figura actual, 0.33-0.55 dispersión, 0.55-0.77 puntos aleatorios, 0.77-1.0 puntos aleatorios->nueva figura
    float interp;
    if (t_phase < 0.33f) {
        n->x = fx;
        n->y = fy;
    } else if (t_phase < 0.55f) {
        float dphase = (t_phase-0.33f)/0.22f;
        interp = 0.5f - 0.5f * cosf(dphase * 3.1415926f);
        n->x = (1.0f - interp) * fx + interp * disp_x;
        n->y = (1.0f - interp) * fy + interp * disp_y;
    } else if (t_phase < 0.77f) {
        float dphase = (t_phase-0.55f)/0.22f;
        interp = 0.5f - 0.5f * cosf(dphase * 3.1415926f);
        n->x = (1.0f - interp) * disp_x + interp * rand_x;
        n->y = (1.0f - interp) * disp_y + interp * rand_y;
    } else {
        float dphase = (t_phase-0.77f)/0.23f;
        interp = 0.5f - 0.5f * cosf(dphase * 3.1415926f);
        n->x = (1.0f - interp) * rand_x + interp * fx_next;
        n->y = (1.0f - interp) * rand_y + interp * fy_next;
    }
    // Efectos únicos por figura (ejemplo: tamaño pulsante, rotación, etc.)
    switch (fig) {
        case 4: // Estrella pulsante
            n->radius = 11.0f + 5.0f * sinf(t_seconds*2.0f + i);
            break;
        case 1: // Flor animada
            n->radius = 10.0f + 4.0f * sinf(t_seconds + i*0.2f);
            break;
        case 5: // Onda
            n->radius = 8.0f + 2.0f * sinf(t_seconds*3.0f + i);
            break;
        default:
            n->radius = 9.0f + 3.0f * sinf(t_seconds + i*0.5f);
    }
 
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
    return;

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