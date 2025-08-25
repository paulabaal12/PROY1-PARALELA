#ifndef ENTITIES_H
#define ENTITIES_H
#include <SDL2/SDL.h>
#include <stdbool.h>

typedef enum { NOTE_QUARTER, NOTE_EIGHTH, NOTE_SIXTEENTH, NOTE_HALF } NoteKind;

typedef struct {
    float x, y;         // posición (centro de la cabeza de la nota)
    float vx, vy;       // velocidad
    float radius;       // radio de la cabeza
    Uint8 r, g, b;      // color base
    NoteKind kind;      // tipo de nota
    float pitch_hz;     // tono asignado (para sonido)
} Note;

// Crea un arreglo de N notas con posiciones, velocidades y colores aleatorios
void notes_init(Note *notes, int N, int width, int height, unsigned int seed);

// Actualiza la posición y color de la nota siguiendo trayectorias creativas (patrones geométricos)
void note_update_creative(Note *n, int i, int N, float t_seconds, int width, int height);

// Renderiza la nota (cabeza, plica y banderas según el tipo)
void note_render(SDL_Renderer *ren, const Note *n);

#endif // ENTITIES_H