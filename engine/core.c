/* core.c — Cngine v2
 * LGPL v3 | Chikatilo / sooborn / SGLauncher */
#include "core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

GameState* game_create(const char* title, int width, int height, float target_fps) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        printf("SDL init failed: %s\n", SDL_GetError());
        return NULL;
    }
    GameState* state = (GameState*)calloc(1, sizeof(GameState));
    if (!state) return NULL;

    state->window = SDL_CreateWindow(title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height, SDL_WINDOW_SHOWN);
    if (!state->window) { free(state); return NULL; }

    state->sdl_renderer = SDL_CreateRenderer(state->window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!state->sdl_renderer) { SDL_DestroyWindow(state->window); free(state); return NULL; }

    state->renderer      = renderer_create(state->sdl_renderer);
    renderer_resize(state->renderer, width, height);
    state->physics_world = physics_world_create(rect(0,0,(float)width,(float)height), 1.f/target_fps);

    state->window_width  = width;
    state->window_height = height;
    state->running       = true;
    state->target_fps    = target_fps;
    state->time_scale    = 1.f;
    state->fps_timer     = SDL_GetTicks();

    return state;
}

static void _input_update(Input* inp, SDL_Event* ev) {
    /* snapshot prev */
    memcpy(inp->keys_prev,     inp->keys,     sizeof(inp->keys));
    memcpy(inp->mouse_btn_prev,inp->mouse_btn,sizeof(inp->mouse_btn));
    memset(inp->keys_pressed,  0, sizeof(inp->keys_pressed));
    memset(inp->keys_released, 0, sizeof(inp->keys_released));
    memset(inp->mouse_btn_pressed, 0, sizeof(inp->mouse_btn_pressed));
    memset(inp->mouse_btn_released,0, sizeof(inp->mouse_btn_released));
    inp->mouse_wheel=0;

    (void)ev; /* events processed in poll loop */
}

static void _input_handle(Input* inp, SDL_Event* ev) {
    switch(ev->type){
        case SDL_KEYDOWN:
            if(ev->key.keysym.scancode<MAX_KEYS){
                if(!inp->keys[ev->key.keysym.scancode])inp->keys_pressed[ev->key.keysym.scancode]=true;
                inp->keys[ev->key.keysym.scancode]=true;
            } break;
        case SDL_KEYUP:
            if(ev->key.keysym.scancode<MAX_KEYS){
                inp->keys_released[ev->key.keysym.scancode]=true;
                inp->keys[ev->key.keysym.scancode]=false;
            } break;
        case SDL_MOUSEMOTION:
            inp->mouse_dx=ev->motion.xrel; inp->mouse_dy=ev->motion.yrel;
            inp->mouse_x=ev->motion.x;     inp->mouse_y=ev->motion.y; break;
        case SDL_MOUSEBUTTONDOWN:
            if(ev->button.button<=3){int b=ev->button.button-1;inp->mouse_btn_pressed[b]=true;inp->mouse_btn[b]=true;} break;
        case SDL_MOUSEBUTTONUP:
            if(ev->button.button<=3){int b=ev->button.button-1;inp->mouse_btn_released[b]=true;inp->mouse_btn[b]=false;} break;
        case SDL_MOUSEWHEEL:
            inp->mouse_wheel=ev->wheel.y; break;
        default: break;
    }
}

void game_run(GameState* state) {
    if (!state) return;
    if (state->callbacks.init) state->callbacks.init(state);

    Uint32 last_time = SDL_GetTicks();
    float frame_ms   = 1000.f / state->target_fps;

    while (state->running) {
        Uint32 now = SDL_GetTicks();
        float  dt  = (now - last_time) / 1000.f;
        if (dt > 0.1f) dt = 0.1f;   /* clamp max delta */
        last_time = now;

        /* FPS counter */
        state->frame_count++;
        if (now - state->fps_timer >= 1000) {
            state->fps = state->frame_count;
            state->frame_count = 0;
            state->fps_timer = now;
        }

        /* Input pre-frame reset */
        _input_update(&state->input, NULL);

        /* Events */
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) state->running = false;
            if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) state->running = false;
            _input_handle(&state->input, &event);
            if (state->callbacks.handle_event) state->callbacks.handle_event(state, &event);
        }

        float scaled_dt = dt * state->time_scale;
        if (!state->paused) state->elapsed_time += scaled_dt;
        state->delta_time = scaled_dt;
        renderer_camera_update(state->renderer, dt);

        /* Physics */
        if (!state->paused && state->physics_world && state->physics_world->particle_count > 0)
            physics_world_update(state->physics_world);

        /* Update */
        if (!state->paused && state->callbacks.update)
            state->callbacks.update(state, scaled_dt);

        /* Render */
        if (state->renderer) {
            renderer_clear(state->renderer);
            if (state->callbacks.render) state->callbacks.render(state, state->renderer);
            renderer_present(state->renderer);
        }

        /* FPS limit (when vsync off) */
        Uint32 elapsed = SDL_GetTicks() - now;
        if (elapsed < (Uint32)frame_ms)
            SDL_Delay((Uint32)frame_ms - elapsed);
    }

    if (state->callbacks.cleanup) state->callbacks.cleanup(state);
}

void game_destroy(GameState* state) {
    if (!state) return;
    if (state->physics_world) physics_world_destroy(state->physics_world);
    if (state->renderer)      renderer_destroy(state->renderer);
    if (state->sdl_renderer)  SDL_DestroyRenderer(state->sdl_renderer);
    if (state->window)        SDL_DestroyWindow(state->window);
    free(state);
    SDL_Quit();
}

void  game_quit(GameState* state)               { if(state) state->running = false; }
void  game_set_time_scale(GameState* s,float t)  { if(s) s->time_scale = t; }
void  game_pause(GameState* s)                   { if(s) s->paused = true; }
void  game_resume(GameState* s)                  { if(s) s->paused = false; }
float game_get_delta_time(GameState* s)          { return s ? s->delta_time : 0.016f; }
float game_get_elapsed(GameState* s)             { return s ? s->elapsed_time : 0; }
int   game_get_fps(GameState* s)                 { return s ? s->fps : 0; }

Vec2 game_mouse_world(GameState* s){
    if(!s) return vec2(0,0);
    return renderer_screen_to_world(s->renderer, vec2((float)s->input.mouse_x,(float)s->input.mouse_y));
}
