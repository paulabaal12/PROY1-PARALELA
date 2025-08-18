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
    srand(seed);
    for (int i=0;i<N;++i){
        Note *n = &notes[i];
        n->radius = 6 + 10*frand01();
        n->x = n->radius + frand01()*(w - 2*n->radius);
        n->y = n->radius + frand01()*(h - 2*n->radius);
        float speed = 80 + 160*frand01();
        float ang = frand01()*6.2831853f;
        n->vx = speed*cosf(ang);
        n->vy = speed*sinf(ang);
        n->r = clampu8(80 + (int)(175*frand01()));
        n->g = clampu8(80 + (int)(175*frand01()));
        n->b = clampu8(80 + (int)(175*frand01()));
        float r = frand01();
        n->kind = (r<0.25f)?NOTE_HALF : (r<0.55f?NOTE_QUARTER : (r<0.85f?NOTE_EIGHTH:NOTE_SIXTEENTH));
        n->pitch_hz = SCALE_HZ[rand()% (int)(sizeof(SCALE_HZ)/sizeof(SCALE_HZ[0]))];
    }
}

bool note_update_bounce(Note *n, float dt, int w, int h) {
    bool bounced = false;
    n->x += n->vx * dt;
    n->y += n->vy * dt;

    const float e = 0.85f; // coeficiente de restitución (pierde energía en rebote)

    if (n->x < n->radius) { n->x = n->radius; n->vx = -n->vx * e; bounced = true; }
    if (n->x > w - n->radius){ n->x = w - n->radius; n->vx = -n->vx * e; bounced = true; }
    if (n->y < n->radius) { n->y = n->radius; n->vy = -n->vy * e; bounced = true; }
    if (n->y > h - n->radius){ n->y = h - n->radius; n->vy = -n->vy * e; bounced = true; }

    return bounced;
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