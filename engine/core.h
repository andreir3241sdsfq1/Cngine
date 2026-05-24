/* core.h — Cngine v2
 * LGPL v3 | Chikatilo / sooborn / SGLauncher
 *
 * v2: input snapshot, delta_time stored, time_scale, pause, FPS counter
 */
#ifndef CORE_H
#define CORE_H

#include "sdl/SDL2/SDL.h"
#include <stdbool.h>
#include "engine_math.h"
#include "physik.h"
#include "render.h"

/* ======================== INPUT ======================== */
#define MAX_KEYS 512
typedef struct {
    bool keys[MAX_KEYS];
    bool keys_prev[MAX_KEYS];
    bool keys_pressed[MAX_KEYS];   /* Нажата в этом кадре */
    bool keys_released[MAX_KEYS];  /* Отпущена в этом кадре */
    int  mouse_x, mouse_y;
    int  mouse_dx, mouse_dy;
    bool mouse_btn[3];
    bool mouse_btn_prev[3];
    bool mouse_btn_pressed[3];
    bool mouse_btn_released[3];
    int  mouse_wheel;
} Input;

static inline bool input_key(Input* inp, SDL_Scancode k){ return inp->keys[k]; }
static inline bool input_key_pressed(Input* inp, SDL_Scancode k){ return inp->keys_pressed[k]; }
static inline bool input_key_released(Input* inp, SDL_Scancode k){ return inp->keys_released[k]; }
static inline bool input_mouse_btn(Input* inp, int b){ return b<3&&inp->mouse_btn[b]; }
static inline bool input_mouse_pressed(Input* inp, int b){ return b<3&&inp->mouse_btn_pressed[b]; }
static inline Vec2 input_mouse_pos(Input* inp){ return vec2((float)inp->mouse_x,(float)inp->mouse_y); }

/* ======================== GAMESTATE ======================== */
typedef struct GameState GameState;

typedef struct {
    void (*init)(GameState* state);
    void (*update)(GameState* state, float dt);
    void (*render)(GameState* state, Renderer* renderer);
    void (*handle_event)(GameState* state, SDL_Event* event);
    void (*cleanup)(GameState* state);
} GameCallbacks;

struct GameState {
    /* SDL */
    SDL_Window*    window;
    SDL_Renderer*  sdl_renderer;
    /* Engine */
    Renderer*      renderer;
    PhysicsWorld*  physics_world;
    Input          input;
    /* Settings */
    int   window_width;
    int   window_height;
    bool  running;
    bool  paused;
    float target_fps;
    float time_scale;
    float delta_time;
    float elapsed_time;
    /* FPS */
    int   fps;
    int   frame_count;
    Uint32 fps_timer;
    /* User data */
    void* user_data;
    /* Callbacks */
    GameCallbacks callbacks;
};

/* ======================== API ======================== */
GameState* game_create(const char* title, int width, int height, float target_fps);
void       game_run(GameState* state);
void       game_destroy(GameState* state);

void  game_quit(GameState* state);
void  game_set_time_scale(GameState* state, float scale);
void  game_pause(GameState* state);
void  game_resume(GameState* state);
float game_get_delta_time(GameState* state);
float game_get_elapsed(GameState* state);
int   game_get_fps(GameState* state);

/* Мировые координаты мыши */
Vec2  game_mouse_world(GameState* state);

#endif /* CORE_H */
