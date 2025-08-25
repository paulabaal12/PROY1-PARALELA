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
    // Alterna entre dos figuras cada 5 segundos
    int mode = ((int)(t_seconds/5.0f))%2;
    float t_phase = fmodf(t_seconds, 5.0f)/5.0f; // 0..1
    float cx = w/2.0f;
    float cy = h*0.60f;
    float interp = 0.5f - 0.5f*cosf(t_phase*3.1415926f); // transición suave

    // Ajuste de cantidad de notas en figura central
    int n_main = (N > 40) ? (int)(N*0.65f) : N;
    int n_head = (int)(n_main*0.68f); // cabeza más densa
    int n_stem = (int)(n_main*0.22f); // plica
    int n_flag = n_main - n_head - n_stem;
    if (n_flag < 2) n_flag = 2;
    float R_head_x = (h<w?w:h)*(N>40?0.13f:0.18f); // radio horizontal
    float R_head_y = R_head_x * 0.7f; // radio vertical para óvalo
    float head_angle = -0.35f; // inclinación de la cabeza
    float x1=0, y1=0;
    if (i < n_main) {
        if (i < n_head) {
            // Cabeza: óvalo inclinado
            float theta = 2*3.1415926f*(float)i/(float)n_head;
            float x_oval = R_head_x * cosf(theta);
            float y_oval = R_head_y * sinf(theta);
            // Rotar el óvalo
            float x_rot = x_oval * cosf(head_angle) - y_oval * sinf(head_angle);
            float y_rot = x_oval * sinf(head_angle) + y_oval * cosf(head_angle);
            x1 = cx + x_rot;
            y1 = cy + y_rot;
        } else if (i < n_head + n_stem) {
            // Plica: línea recta, bien alineada al borde derecho del óvalo
            float frac = (float)(i-n_head)/(float)(n_stem-1);
            float px = cx + R_head_x * cosf(head_angle);
            float py = cy + R_head_x * sinf(head_angle);
            x1 = px + 7.0f; // pequeño offset para que no se superponga
            y1 = py - frac*R_head_x*2.1f;
        } else {
            // Bandera: curva elegante
            float frac = (float)(i-n_head-n_stem)/(float)(n_flag-1);
            float px = cx + R_head_x * cosf(head_angle) + 7.0f;
            float py = cy + R_head_x * sinf(head_angle) - R_head_x*2.1f;
            float bx = px + frac*R_head_x*1.0f;
            float by = py - 0.5f*R_head_x + sinf(frac*3.1415f)*R_head_x*0.7f;
            x1 = bx;
            y1 = by;
        }
    } else {
        // Notas libres: trayectorias creativas alrededor
        float t = t_seconds*0.7f + i;
        float r = R_head_x*2.2f + 40.0f*sinf(t*0.5f+i);
        float theta = 2*3.1415926f*(float)(i-n_main)/(float)(N-n_main) + t*0.25f;
        x1 = cx + r*cosf(theta);
        y1 = cy + r*sinf(theta);
    }
    // Cabeza
    float x2=0, y2=0;
    if (i < n_main) {
        if (i < n_head) {
            float theta = 2*3.1415926f*(float)i/(float)n_head + 0.5f*sinf(t_seconds*1.2f);
            float r_mod = R_head_x * 1.08f;
            float x_oval = r_mod * cosf(theta);
            float y_oval = (R_head_y * 1.08f) * sinf(theta);
            float x_rot = x_oval * cosf(head_angle) - y_oval * sinf(head_angle);
            float y_rot = x_oval * sinf(head_angle) + y_oval * cosf(head_angle);
            x2 = cx + x_rot;
            y2 = cy + y_rot;
        } else if (i < n_head + n_stem) {
            float frac = (float)(i-n_head)/(float)(n_stem-1);
            float px = cx + R_head_x * cosf(head_angle);
            float py = cy + R_head_x * sinf(head_angle);
            x2 = px + 7.0f;
            y2 = py - frac*R_head_x*2.1f;
        } else {
            float frac = (float)(i-n_head-n_stem)/(float)(n_flag-1);
            float px = cx + R_head_x * cosf(head_angle) + 7.0f;
            float py = cy + R_head_x * sinf(head_angle) - R_head_x*2.1f;
            float bx = px + frac*R_head_x*1.0f;
            float by = py - 0.5f*R_head_x + sinf(frac*3.1415f)*R_head_x*0.7f;
            x2 = bx;
            y2 = by + 0.7f*R_head_x;
        }
    } else {
        // Notas libres: trayectorias creativas alrededor
        float t = t_seconds*0.7f + i;
    float r = R_head_x*2.2f + 40.0f*sinf(t*0.5f+i);
        float theta = 2*3.1415926f*(float)(i-n_main)/(float)(N-n_main) + t*0.25f + 1.5f;
        x2 = cx + r*cosf(theta);
        y2 = cy + r*sinf(theta);
    }

    // Interpolación entre ambas figuras
    float x = mode ? (1.0f-interp)*x1 + interp*x2 : (1.0f-interp)*x2 + interp*x1;
    float y = mode ? (1.0f-interp)*y1 + interp*y2 : (1.0f-interp)*y2 + interp*y1;
    n->x = x;
    n->y = y;

    // Color por sección
    if (i < n_main) {
        if (i < n_head) {
            // Degradado en la cabeza
            float grad = (float)i/(float)n_head;
            n->r = (Uint8)(220 - 40*grad);
            n->g = (Uint8)(60 + 120*grad);
            n->b = (Uint8)(180 + 40*grad);
            n->radius = (N>40) ? 10.0f : 18.0f;
        } else if (i < n_head + n_stem) {
            n->r = 60; n->g = 220; n->b = 120; // plica: verde
            n->radius = (N>40) ? 7.0f : 13.0f;
        } else {
            n->r = 60; n->g = 120; n->b = 220; // banderas: azul
            n->radius = (N>40) ? 7.0f : 13.0f;
        }
    } else {
        // Notas libres: solo círculos pequeños, color arcoíris
        float hue = fmodf((t_seconds*30.0f + i*360.0f/N), 360.0f);
        float s = 0.7f, v = 0.85f;
        float C = v*s, X = C*(1 - fabsf(fmodf(hue/60.0f,2.0f)-1)), m = v-C;
        float rr=0,gg=0,bb=0;
        if(hue<60){rr=C;gg=X;bb=0;}else if(hue<120){rr=X;gg=C;bb=0;}else if(hue<180){rr=0;gg=C;bb=X;}else if(hue<240){rr=0;gg=X;bb=C;}else if(hue<300){rr=X;gg=0;bb=C;}else{rr=C;gg=0;bb=X;}
        n->r = clampu8((int)((rr+m)*255));
        n->g = clampu8((int)((gg+m)*255));
        n->b = clampu8((int)((bb+m)*255));
        n->radius = 7.0f;
    }
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