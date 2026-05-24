# Cngine v2

> A lightweight 2D game engine written in C, powered by SDL2 and Lua 5.4.

**LGPL v3** — use freely in commercial and open-source projects.  
Authors: Chikatilo / sooborn / SGLauncher

---

## Features

- **Pixel-buffer renderer** — 320×200 retro aesthetic, scales to any window size
- **Particle physics** — PBD solver, Verlet integration, spatial hash O(n) collisions
- **Soft bodies & cloth** — springs, breakable joints, pressure simulation
- **Hitbox system** — AABB, Circle, OBB, Capsule, Polygon, Composite
- **Lua 5.4 scripting** — hot-reload scripts without restarting the game
- **Software audio mixer** — 32 channels, positional 2D audio, fade/crossfade
- **2D dynamic lighting** — radius lights, flicker, ambient
- **Post-effects** — bloom, vignette, chromatic aberration, CRT, heat haze, scanlines, and more
- **Weather & particle FX** — fire, smoke, rain, snow, lightning, ripples, metaballs
- **Tilemap & animated sprites** — layers, parallax scrolling, clip-based animation

---

## Project Structure

```
Cngine/
├── game.c                        ← your game code (entry point)
├── cmp.bat                       ← Windows build script
├── scripts/
│   └── example_script.lua        ← example Lua script
└── engine/
    ├── core.h / core.c           ← game loop, window, input
    ├── render.h / render.c       ← pixel renderer, camera, lighting, FX
    ├── physik.h / physik.c       ← particle physics, cloth, soft bodies
    ├── hitboxes.h / hitboxes.c   ← collision shapes and detection
    ├── audio.h / audio.c         ← software audio mixer
    ├── scripts.h / scripts.c     ← Lua 5.4 integration
    ├── engine_math.h / .c        ← Vec2, Mat4, AABB, noise, easing, bezier
    ├── postfx_config.h / .c      ← post-processing presets
    ├── lua/                      ← Lua 5.4 source (included)
    └── sdl/                      ← SDL2 headers + prebuilt libs (MinGW)
```

---

## Building (Windows)

**Requirements:**
- [w64devkit](https://github.com/skeeto/w64devkit/releases) (MinGW-w64 GCC) — `gcc` must be in `PATH`
- `SDL2.dll` next to `game.exe` — grab from [SDL releases](https://github.com/libsdl-org/SDL/releases)

**Build:**
```bat
cmp.bat
```

This compiles Lua 5.4, all engine modules, and your `game.c`, then links them into `game.exe`.

> **Linux/macOS:** install SDL2 via your package manager, then compile each `.c` file with `gcc -I. -Ilua -lSDL2 -lm`.

---

## Quick Start

A minimal game in `game.c`:

```c
#include "engine/core.h"
#include "engine/render.h"

typedef struct { float x, y; } Player;

void my_init(GameState* gs) {
    Player* p = gs->user_data;
    p->x = gs->window_width  / 2.0f;
    p->y = gs->window_height / 2.0f;
}

void my_update(GameState* gs, float dt) {
    Player* p = gs->user_data;
    if (input_key(&gs->input, SDL_SCANCODE_RIGHT)) p->x += 120 * dt;
    if (input_key(&gs->input, SDL_SCANCODE_LEFT))  p->x -= 120 * dt;
    if (input_key(&gs->input, SDL_SCANCODE_DOWN))  p->y += 120 * dt;
    if (input_key(&gs->input, SDL_SCANCODE_UP))    p->y -= 120 * dt;
}

void my_render(GameState* gs, Renderer* r) {
    Player* p = gs->user_data;
    renderer_draw_circle(r, vec2(p->x, p->y), 8, COLOR_CYAN, true);
    renderer_draw_text(r, "WASD to move", vec2(4, 4), COLOR_WHITE, 1.0f);
}

int main(void) {
    Player player = {0};
    GameState* gs = game_create("My Game", 640, 400, 60);
    gs->user_data         = &player;
    gs->callbacks.init    = my_init;
    gs->callbacks.update  = my_update;
    gs->callbacks.render  = my_render;
    game_run(gs);
    game_destroy(gs);
    return 0;
}
```

See [`game.c`](game.c) for a full example using audio, hitboxes, Lua scripts, and post-effects.

---

## Module Overview

### `engine/core.h` — Game Loop & Input

```c
GameState* gs = game_create("Title", 640, 400, 60.0f);
gs->user_data = &my_data;
gs->callbacks.init    = on_init;
gs->callbacks.update  = on_update;   // float dt
gs->callbacks.render  = on_render;   // Renderer* r
gs->callbacks.cleanup = on_cleanup;
game_run(gs);
game_destroy(gs);

// Input
input_key(&gs->input, SDL_SCANCODE_SPACE)      // held
input_key_pressed(&gs->input, SDL_SCANCODE_A)  // this frame only
input_mouse_pos(&gs->input)                    // → Vec2
game_mouse_world(gs)                           // mouse in world coords

// Control
game_set_time_scale(gs, 0.5f);   // slow motion
game_pause(gs);
game_get_fps(gs);                 // → int
```

---

### `engine/render.h` — Renderer

```c
// Primitives
renderer_draw_circle(r, vec2(x, y), radius, COLOR_RED, filled);
renderer_draw_rect(r, rect(x, y, w, h), COLOR_WHITE, false);
renderer_draw_line(r, vec2(x1,y1), vec2(x2,y2), COLOR_YELLOW);
renderer_draw_text(r, "Hello!", vec2(4, 4), COLOR_WHITE, 1.0f);

// Camera
renderer_shake_camera(r, 0.6f);              // trauma 0-1
renderer_camera_follow(r, target_vec2, dt);  // smooth follow
renderer_zoom_camera(r, 1.5f);

// Dynamic lights
int id = renderer_add_light(r, pos, COLOR_ORANGE, 80.0f, 1.2f);
renderer_set_light_flicker(r, id, 0.3f);
r->lighting_enabled = true;
r->ambient_light = color_rgb(10, 10, 30);

// Post-effects (C)
r->postfx.bloom           = true;
r->postfx.bloom_strength  = 1.5f;
r->postfx.vignette        = true;
r->postfx.film_grain      = true;
r->postfx.grain_strength  = 0.04f;

// Post-effects (presets via postfx_config.h)
postfx_apply_preset(r, PFX_PRESET_CINEMATIC);
postfx_apply_preset(r, PFX_PRESET_HORROR);
postfx_apply_preset(r, PFX_PRESET_NEON);
```

Available presets: `PFX_PRESET_NONE`, `RETRO`, `CINEMATIC`, `HORROR`, `NEON`

---

### `engine/physik.h` — Physics

```c
PhysicsWorld* world = gs->physics_world; // created by game_create

// Particles
Particle* ball = particle_create(world, vec2(100, 50), 1.0f, 6.0f, PARTICLE_TYPE_NORMAL);
Particle* wall = particle_create(world, vec2(0, 200),  0.0f, 0.0f, PARTICLE_TYPE_STATIC);
particle_apply_force(ball, vec2(0, 200));
particle_apply_impulse(ball, vec2(50, -100));

// Springs
Spring* s = spring_create(world, a, b, 200.0f, 0.5f);
spring_create_breakable(world, a, b, 150.0f, 0.3f, 800.0f); // breaks at 800N

// Cloth
Cloth* cloth = cloth_create(world, vec2(50,20), vec2(200,120), 12, 8, 0.1f);
cloth_pin(cloth, 0, 0);   // pin top-left corner
cloth_pin(cloth, 11, 0);  // pin top-right corner

// Soft body
SoftBody* blob = softbody_create_circle(world, vec2(160,100), 30.0f, 12, 1.0f, 400.0f);

// Explosion
physics_explosion(world, vec2(160, 200), 80.0f, 500.0f);

// Raycast
RaycastHit hit = physics_raycast(world, vec2(0,0), vec2(1,0), 300.0f, LAYER_ALL);
if (hit.hit) { /* hit.point, hit.normal, hit.particle */ }

// Force fields
ForceField* vortex = force_field_create(world, FORCE_FIELD_VORTEX, vec2(160,100), 80.0f, 3.0f);
ForceField* wind   = force_field_create(world, FORCE_FIELD_WIND_ZONE, vec2(0,0), 0, 1.5f);
force_field_set_direction(wind, vec2(1, 0));

// World settings
physics_world_set_gravity(world, vec2(0, 600));
physics_world_set_substeps(world, 3);   // more = more accurate
```

---

### `engine/hitboxes.h` — Hitboxes

```c
HitboxManager* hbm = hbm_create();

// Create shapes
Hitbox* sword   = hb_create_capsule(4.0f, 16.0f);
Hitbox* pickup  = hb_create_circle(20.0f);
pickup->is_trigger = true;    // detection only, no physics push
pickup->owner = &my_item;

// Add to manager
int sword_idx  = hbm_add(hbm, sword,  player_pos, player_angle);
int pickup_idx = hbm_add(hbm, pickup, item_pos, 0.0f);

// Update transform every frame
hbm_update_transform(hbm, sword_idx, new_pos, new_angle);

// Process callbacks & queries
hbm_process(hbm);

// Query
HitResult results[8];
int n = hbm_query_hitbox(hbm, sword, sword_pos, sword_angle, hit_idx, results, 8);

// Debug render
hbm_debug_draw_all(r, hbm, COLOR_GREEN, COLOR_YELLOW);
```

---

### `engine/audio.h` — Audio

```c
AudioSystem* audio = audio_create();

// Load & play
audio_load_sound(audio, "assets/jump.wav", "jump");
audio_load_music(audio, "assets/theme.wav", "theme");

audio_play(audio, "jump", 1.0f, 1.0f, 0.0f);         // volume, pitch, pan
audio_play_at(audio, "explosion", wx, wy, 1.0f);      // positional
audio_play_music(audio, "theme", 0.8f, true);
audio_crossfade_music(audio, "boss_theme", 2.0f);

// Named events (randomized pitch/volume)
audio_register_event(audio, "hit", "hit_sfx", 0.7f, 1.0f, 0.9f, 1.2f, 0.05f);
audio_trigger(audio, "hit");

// Update each frame (required for positional audio)
audio_update(audio, dt);
audio_set_listener(audio, listener_x, listener_y, 400.0f);

// Volume
audio_set_master_volume(audio, 0.8f);
audio_set_music_volume(audio, 0.5f);
```

---

### `engine/scripts.h` — Lua Scripting

```c
ScriptManager* sm = scripts_create(gs, audio, hbm);
int idx = scripts_load(sm, "scripts/player.lua", "player");
scripts_watch(sm, idx, true);   // auto-reload on file change

// Each frame:
scripts_check_watches(sm, dt);
scripts_call_update(sm, idx, dt);

// Pass data C → Lua
scripts_set_global_int(sm, idx, "score", 42);
scripts_set_global_float(sm, idx, "speed", 200.0f);

// Read data Lua → C
int score = scripts_get_global_int(sm, idx, "score");
```

**Lua script structure** (`scripts/example_script.lua`):

```lua
function on_init()
    render.set_clear_color(20, 20, 40)
    render.postfx_vignette(true, 0.4)
end

function on_update(dt)
    if engine.key_down(Key.RIGHT) then player.x = player.x + 80 * dt end
    if engine.key_pressed(Key.SPACE) then
        render.camera_shake(0.5)
        audio.trigger("jump")
    end
end

function on_render()
    render.draw_circle(player.x, player.y, 6, 100, 200, 255, 255, true)
    render.draw_text("FPS: " .. engine.fps(), 4, 4, 255, 255, 100, 255, 1.0)
end

function on_event(type)
    if type == "SDL_QUIT" then engine.quit() end
end
```

**Available Lua modules:** `engine`, `render`, `audio`, `phys`, `hitbox`, `Key`

---

### `engine/engine_math.h` — Math

```c
// Vec2
Vec2 v = vec2(10, 20);
vec2_add(a, b);  vec2_sub(a, b);  vec2_mul(v, 2.0f);
vec2_normalize(v);  vec2_length(v);  vec2_distance(a, b);
vec2_lerp(a, b, t);  vec2_rotate(v, angle);  vec2_reflect(v, normal);
vec2_smooth_damp(cur, target, &vel, smooth_time, dt);

// AABB
AABB box = aabb(x0, y0, x1, y1);
aabb_overlaps(a, b);  aabb_contains(a, point);  aabb_union(a, b);

// Easing (24 functions)
ease_in_quad(t);  ease_out_elastic(t);  ease_in_out_cubic(t);

// Noise
simplex2(x, y);       // 2D simplex noise → [-1, 1]
worley(x, y, seed);   // Worley/cell noise

// Bezier
bezier_quad(p0, p1, p2, t);
bezier_cubic(p0, p1, p2, p3, t);
catmull_rom(p0, p1, p2, p3, t);
```

---

## API Reference

Full API documentation is available in the repo:

- [`ApiENG.txt`](ApiENG.txt) — complete English reference
- [`ApiRU.txt`](ApiRU.txt) — полная документация на русском

---

## License

**LGPL v3** — see [`LICENSE`](LICENSE).

You can use Cngine in proprietary games **without releasing your game's source code**, as long as you allow relinking against modified engine versions. Any changes to the engine files themselves must be released under LGPL v3.
