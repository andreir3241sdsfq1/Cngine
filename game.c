/* game.c — Cngine v2 example
 * LGPL v3 | Chikatilo / sooborn / SGLauncher
 *
 * Показывает использование:
 *   - Scripts (Lua)
 *   - Audio
 *   - Hitboxes
 *   - PostFX config
 */
#include "engine/core.h"
#include "engine/audio.h"
#include "engine/hitboxes.h"
#include "engine/scripts.h"
#include "engine/postfx_config.h"
#include <stdio.h>

/* ================================================================
   ДАННЫЕ ИГРЫ
================================================================ */
typedef struct {
    AudioSystem*   audio;
    HitboxManager* hbm;
    ScriptManager* scripts;
    int            main_script_idx;
} GameData;

/* ================================================================
   CALLBACKS
================================================================ */
static void game_init(GameState* state){
    GameData* gd = (GameData*)state->user_data;

    /* --- AUDIO --- */
    gd->audio = audio_create();
    if(gd->audio){
        /* Загрузить звуки (замените на реальные WAV файлы) */
        /* audio_load_sound(gd->audio, "assets/jump.wav", "jump_sfx"); */
        /* audio_load_music(gd->audio, "assets/music.wav", "main_theme"); */

        /* Зарегистрировать событие */
        /* audio_register_event(gd->audio, "jump", "jump_sfx", 0.8f,1.0f, 0.9f,1.1f, 0.1f); */

        audio_set_master_volume(gd->audio, 1.0f);
        audio_set_listener(gd->audio,
            (float)state->window_width  * 0.5f,
            (float)state->window_height * 0.5f,
            500.f);
    }

    /* --- HITBOXES --- */
    gd->hbm = hbm_create();

    /* Пример: добавить хитбокс стены */
    Hitbox* wall = hb_create_aabb(50.f, 10.f);
    wall->is_trigger = false;
    hbm_add(gd->hbm, wall, vec2(160.f, 180.f), 0.f);

    /* Пример: хитбокс-триггер */
    Hitbox* trigger = hb_create_circle(30.f);
    trigger->is_trigger = true;
    hbm_add(gd->hbm, trigger, vec2(100.f, 100.f), 0.f);

    /* --- POSTFX --- */
    /* Вариант А: пресет */
    postfx_apply_preset(state->renderer, PFX_PRESET_CINEMATIC);

    /* Вариант Б: тонкая настройка вручную */
    /* postfx_disable_all(state->renderer); */
    /* postfx_enable_bloom(state->renderer, true, 4, 1.5f, 0.75f); */
    /* postfx_enable_vignette(state->renderer, true, 0.5f, COLOR_BLACK); */

    /* --- SCRIPTS --- */
    gd->scripts = scripts_create(state, gd->audio, gd->hbm);
    gd->main_script_idx = scripts_load(gd->scripts, "scripts/example_script.lua", "main");
    if(gd->main_script_idx >= 0){
        scripts_watch(gd->scripts, gd->main_script_idx, true); /* горячая перезагрузка */
        scripts_call_init(gd->scripts, gd->main_script_idx);
    }

    printf("[Game] Init complete\n");
}

static void game_update(GameState* state, float dt){
    GameData* gd = (GameData*)state->user_data;

    /* Аудио */
    if(gd->audio){
        audio_update(gd->audio, dt);
        audio_set_listener(gd->audio,
            (float)state->input.mouse_x,
            (float)state->input.mouse_y,
            500.f);
    }

    /* Хитбоксы */
    if(gd->hbm) hbm_process(gd->hbm);

    /* Скрипты — обновление + проверка горячей перезагрузки */
    if(gd->scripts){
        scripts_check_watches(gd->scripts, dt);
        scripts_call_update(gd->scripts, gd->main_script_idx, dt);
    }

    /* Умный bloom на светящиеся пиксели (вызвать ПЕРЕД presenter_present) */
    /* postfx_smart_bloom(state->renderer, 4, 1.2f, true); */
}

static void game_render(GameState* state, Renderer* r){
    GameData* gd = (GameData*)state->user_data;

    /* Скрипт рисует сам */
    if(gd->scripts) scripts_call_render(gd->scripts, gd->main_script_idx);

    /* Debug хитбоксов */
    if(gd->hbm){
        Color active_col  = color_rgba(0, 255, 80, 200);
        Color trigger_col = color_rgba(255, 200, 0, 180);
        hbm_debug_draw_all(r, gd->hbm, active_col, trigger_col);
    }

    /* FPS поверх */
    char buf[32];
    snprintf(buf, sizeof(buf), "FPS: %d", game_get_fps(state));
    renderer_draw_text(r, buf, vec2(4,4), color_rgb(255,255,100), 1.f);
}

static void game_handle_event(GameState* state, SDL_Event* ev){
    GameData* gd = (GameData*)state->user_data;
    if(!gd->scripts) return;
    /* Передать тип события в Lua */
    const char* ev_str = "SDL_UNKNOWN";
    switch(ev->type){
        case SDL_KEYDOWN:  ev_str="SDL_KEYDOWN";  break;
        case SDL_KEYUP:    ev_str="SDL_KEYUP";    break;
        case SDL_MOUSEBUTTONDOWN: ev_str="SDL_MOUSEBUTTONDOWN"; break;
        case SDL_QUIT:     ev_str="SDL_QUIT";     break;
        default: break;
    }
    scripts_call_event(gd->scripts, gd->main_script_idx, ev_str);
}

static void game_cleanup(GameState* state){
    GameData* gd = (GameData*)state->user_data;
    if(gd->scripts) scripts_destroy(gd->scripts);
    if(gd->hbm)     hbm_destroy(gd->hbm);
    if(gd->audio)   audio_destroy(gd->audio);
    printf("[Game] Cleanup complete\n");
}

/* ================================================================
   MAIN
================================================================ */
int main(int argc, char* argv[]){
    GameData gd = {0};

    GameState* state = game_create("Cngine v2", 640, 400, 60.f);
    if(!state){ printf("Failed to create game\n"); return 1; }

    state->user_data = &gd;
    state->callbacks.init         = game_init;
    state->callbacks.update       = game_update;
    state->callbacks.render       = game_render;
    state->callbacks.handle_event = game_handle_event;
    state->callbacks.cleanup      = game_cleanup;

    game_run(state);
    game_destroy(state);
    return 0;
}
