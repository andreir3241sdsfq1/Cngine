/* scripts.h — Cngine v2
 * LGPL v3 | Chikatilo / sooborn / SGLauncher
 *
 * Lua 5.4 скриптинг:
 *   - Загрузка и выполнение .lua файлов
 *   - Вызов Lua-функций из C (on_init, on_update, on_render, on_event)
 *   - Биндинги к движку: core, render, physics, audio, hitboxes
 *   - Горячая перезагрузка скриптов
 *   - Именованные компоненты-скрипты
 */
#ifndef SCRIPTS_H
#define SCRIPTS_H

/* Lua headers */
#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

#include "core.h"
#include "audio.h"
#include "hitboxes.h"
#include <stdbool.h>

/* ================================================================
   КОНФИГУРАЦИЯ
================================================================ */
#define SCRIPTS_MAX         32      /* максимум загруженных скриптов */
#define SCRIPTS_MAX_WATCHES 32      /* файлы для горячей перезагрузки */
#define SCRIPTS_STACK_SIZE  (512*1024) /* размер стека Lua */

/* ================================================================
   СКРИПТ
================================================================ */
typedef struct {
    lua_State* L;             /* отдельное состояние Lua */
    char       path[256];     /* путь к файлу */
    char       name[64];      /* имя компонента */
    bool       loaded;
    bool       has_init;
    bool       has_update;
    bool       has_render;
    bool       has_event;
    bool       has_destroy;
    /* Горячая перезагрузка */
    long       last_mtime;
    bool       watch;
} Script;

/* ================================================================
   МЕНЕДЖЕР СКРИПТОВ
================================================================ */
typedef struct {
    Script     scripts[SCRIPTS_MAX];
    int        count;
    /* Глобальный Lua state (для shared API) */
    lua_State* global_L;
    /* Ссылки на системы движка */
    GameState* game;
    AudioSystem* audio;
    HitboxManager* hbm;
    /* Время для watch */
    float      watch_timer;
    float      watch_interval; /* секунд между проверками */
} ScriptManager;

/* ================================================================
   ИНИЦИАЛИЗАЦИЯ
================================================================ */
ScriptManager* scripts_create(GameState* game, AudioSystem* audio, HitboxManager* hbm);
void           scripts_destroy(ScriptManager* sm);

/* ================================================================
   ЗАГРУЗКА СКРИПТОВ
================================================================ */
/* Загрузить скрипт, выполнить тело файла (регистрирует функции).
   Возвращает индекс или -1 при ошибке. */
int  scripts_load(ScriptManager* sm, const char* path, const char* name);

/* Перезагрузить скрипт (горячая перезагрузка) */
bool scripts_reload(ScriptManager* sm, int idx);

/* Включить автоматическую перезагрузку при изменении файла */
void scripts_watch(ScriptManager* sm, int idx, bool enabled);

/* Выгрузить скрипт */
void scripts_unload(ScriptManager* sm, int idx);

/* Найти скрипт по имени */
int  scripts_find(ScriptManager* sm, const char* name);

/* ================================================================
   ВЫЗОВЫ ИЗ C → LUA
================================================================ */
/* Вызвать on_init(game_state_table) */
void scripts_call_init(ScriptManager* sm, int idx);

/* Вызвать on_update(dt) */
void scripts_call_update(ScriptManager* sm, int idx, float dt);

/* Вызвать on_render() */
void scripts_call_render(ScriptManager* sm, int idx);

/* Вызвать on_event(event_type_str, data_table) */
void scripts_call_event(ScriptManager* sm, int idx, const char* event_type);

/* Вызвать on_destroy() */
void scripts_call_destroy(ScriptManager* sm, int idx);

/* Вызвать все скрипты сразу */
void scripts_update_all(ScriptManager* sm, float dt);
void scripts_render_all(ScriptManager* sm);
void scripts_init_all(ScriptManager* sm);
void scripts_destroy_all(ScriptManager* sm);

/* Проверить файлы на изменения (для горячей перезагрузки) */
void scripts_check_watches(ScriptManager* sm, float dt);

/* ================================================================
   ВЫЗОВЫ ИЗ LUA → C (встроенные функции)
   Доступны в Lua автоматически после scripts_create()
================================================================ */
/* После загрузки в глобальном состоянии доступны:

   -- CORE / GAME
   engine.quit()
   engine.pause(bool)
   engine.set_time_scale(float)
   engine.dt()                    -> float
   engine.fps()                   -> int
   engine.elapsed()               -> float
   engine.width()                 -> int
   engine.height()                -> int
   engine.mouse_x(), mouse_y()    -> float
   engine.key_down(scancode)      -> bool
   engine.key_pressed(scancode)   -> bool

   -- RENDER
   render.clear()
   render.draw_rect(x, y, w, h, r, g, b, a, filled)
   render.draw_circle(x, y, radius, r, g, b, a, filled)
   render.draw_line(x1,y1,x2,y2, r,g,b,a)
   render.draw_text(text, x, y, r, g, b, a, scale)
   render.set_clear_color(r,g,b)
   render.camera_set(x, y)
   render.camera_zoom(factor)
   render.camera_shake(trauma)

   -- POST FX
   render.postfx_bloom(on, radius, strength, threshold)
   render.postfx_vignette(on, strength)
   render.postfx_grain(on, strength)
   render.postfx_preset(name)    -- "retro","cinematic","horror","neon","none"

   -- AUDIO
   audio.load(path, name)
   audio.play(name, volume, pitch, pan)   -> channel_id
   audio.play_at(name, x, y, volume)      -> channel_id
   audio.play_music(name, volume, loop)
   audio.stop_music()
   audio.crossfade(name, seconds)
   audio.trigger(event_name)
   audio.set_master(volume)
   audio.set_music_vol(volume)
   audio.set_sfx_vol(volume)
   audio.register_event(trigger, sound, vmin, vmax, pmin, pmax, cooldown)

   -- PHYSICS
   phys.set_gravity(x, y)
   phys.add_particle(x, y, mass, radius)  -> particle_id
   phys.apply_force(particle_id, fx, fy)
   phys.explosion(x, y, radius, force)
   phys.raycast(ox, oy, dx, dy, dist)     -> hit, px, py, nx, ny

   -- HITBOXES (если hbm передан)
   hitbox.add_aabb(x, y, hw, hh)          -> idx
   hitbox.add_circle(x, y, radius)        -> idx
   hitbox.update(idx, x, y)
   hitbox.remove(idx)
   hitbox.query_point(x, y)               -> array of idx
*/

/* ================================================================
   НИЗКОУРОВНЕВЫЙ ДОСТУП
================================================================ */
/* Получить lua_State скрипта */
lua_State* scripts_get_state(ScriptManager* sm, int idx);
/* Получить глобальный lua_State */
lua_State* scripts_global_state(ScriptManager* sm);
/* Выполнить строку кода в контексте скрипта */
bool scripts_exec_string(ScriptManager* sm, int idx, const char* code);
/* Выполнить строку в глобальном контексте */
bool scripts_exec_global(ScriptManager* sm, const char* code);
/* Установить глобальную переменную в скрипте */
void scripts_set_global_int(ScriptManager* sm, int idx, const char* name, int val);
void scripts_set_global_float(ScriptManager* sm, int idx, const char* name, float val);
void scripts_set_global_string(ScriptManager* sm, int idx, const char* name, const char* val);
/* Получить глобальную переменную из скрипта */
int   scripts_get_global_int(ScriptManager* sm, int idx, const char* name);
float scripts_get_global_float(ScriptManager* sm, int idx, const char* name);

#endif /* SCRIPTS_H */
