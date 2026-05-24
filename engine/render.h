/* render.h — Cngine v2
 * LGPL v3 | Chikatilo / sooborn / SGLauncher */
#ifndef RENDER_H
#define RENDER_H

#include "sdl/SDL2/SDL.h"
#include <stdbool.h>
#include "engine_math.h"
#include "physik.h"

/* ================================================================
   РАЗРЕШЕНИЕ ПИКСЕЛЬНОГО БУФЕРА
================================================================ */
#ifndef PB_W
#define PB_W 320
#endif
#ifndef PB_H
#define PB_H 200
#endif

/* ================================================================
   ЦВЕТ
================================================================ */
typedef struct { Uint8 r,g,b,a; } Color;

static inline Color color_rgb(Uint8 r,Uint8 g,Uint8 b)          { Color c={r,g,b,255}; return c; }
static inline Color color_rgba(Uint8 r,Uint8 g,Uint8 b,Uint8 a) { Color c={r,g,b,a};   return c; }
static inline Color color_lerp(Color a,Color b,float t){
    return color_rgba((Uint8)(a.r+(b.r-a.r)*t),(Uint8)(a.g+(b.g-a.g)*t),(Uint8)(a.b+(b.b-a.b)*t),(Uint8)(a.a+(b.a-a.a)*t));
}
static inline Color color_mul(Color c,float f){
    int r2=(int)(c.r*f),g=(int)(c.g*f),b=(int)(c.b*f);
    if(r2>255)r2=255; if(r2<0)r2=0;
    if(g>255)g=255;   if(g<0)g=0;
    if(b>255)b=255;   if(b<0)b=0;
    return color_rgba((Uint8)r2,(Uint8)g,(Uint8)b,c.a);
}
static inline Color color_add(Color a,Color b){
    int r2=a.r+b.r,g=a.g+b.g,bv=a.b+b.b;
    return color_rgba(r2>255?255:r2,g>255?255:g,bv>255?255:bv,a.a);
}
static inline Color color_from_hsv(float h,float s,float v){
    RGB rgb=hsv_to_rgb((HSV){h,s,v});
    return color_rgb((Uint8)(rgb.r*255),(Uint8)(rgb.g*255),(Uint8)(rgb.b*255));
}
static inline Color color_from_u32(Uint32 hex){
    return color_rgba((hex>>24)&0xFF,(hex>>16)&0xFF,(hex>>8)&0xFF,hex&0xFF);
}
static inline Uint32 color_to_u32(Color c){ return ((Uint32)c.r<<24)|((Uint32)c.g<<16)|((Uint32)c.b<<8)|c.a; }
static inline Color  color_invert(Color c){ return color_rgba(255-c.r,255-c.g,255-c.b,c.a); }
static inline Color  color_with_alpha(Color c,Uint8 a){ c.a=a; return c; }

extern const Color COLOR_WHITE, COLOR_BLACK, COLOR_RED, COLOR_GREEN,
                   COLOR_BLUE, COLOR_YELLOW, COLOR_MAGENTA, COLOR_CYAN,
                   COLOR_GRAY, COLOR_ORANGE, COLOR_DARKGRAY, COLOR_TRANSPARENT,
                   COLOR_PURPLE, COLOR_TEAL, COLOR_LIME, COLOR_PINK,
                   COLOR_BROWN, COLOR_NAVY, COLOR_MAROON, COLOR_OLIVE;

/* Градиент (4 угла) */
typedef struct { Color tl,tr,bl,br; } Gradient;
static inline Gradient gradient_horizontal(Color left,Color right){ return (Gradient){left,right,left,right}; }
static inline Gradient gradient_vertical(Color top,Color bot)     { return (Gradient){top,top,bot,bot}; }
static inline Gradient gradient_solid(Color c)                    { return (Gradient){c,c,c,c}; }

/* ================================================================
   ТЕКСТУРА
================================================================ */
typedef struct {
    SDL_Texture* texture;
    int          width, height, channels;
} Texture;

typedef struct { int x,y,w,h; } TexRect;

/* ================================================================
   КАМЕРА
================================================================ */
typedef struct {
    Vec2  position;
    float zoom;
    float rotation;
    float shake_intensity;
    float shake_timer;
    float shake_frequency;
    float trauma;          /* 0-1 trauma-based shake */
    float trauma_decay;    /* скорость затухания */
    float shake_x, shake_y; /* текущий оффсет тряски */
    Vec2  velocity;
    float smooth_speed;
} Camera;

/* ================================================================
   ПОСТЭФФЕКТЫ
================================================================ */
typedef struct {
    /* Bloom */
    bool  bloom;
    int   bloom_radius;
    float bloom_strength;
    float bloom_threshold;

    /* Виньетка */
    bool  vignette;
    float vignette_strength;
    Color vignette_color;

    /* Хроматическая аберрация */
    bool  chromatic_aberration;
    float ca_strength;

    /* Scanlines */
    bool  scanlines;
    float scanlines_alpha;
    float scanlines_strength;
    int   scanlines_gap;

    /* Пикселизация */
    bool  pixelate;
    int   pixel_size;

    /* Цветокоррекция */
    bool  color_grade;
    float saturation;
    float contrast;
    float brightness;
    float gamma;
    Color color_tint;

    /* Зернистость */
    bool  film_grain;
    float grain_strength;
    float film_grain_strength; /* алиас grain_strength */

    /* Тепловое марево */
    bool  heat_haze;
    float heat_intensity;
    float heat_speed;

    /* Подводный */
    bool  underwater;
    float underwater_distort;
    Color underwater_tint;

    /* Night vision */
    bool  night_vision;
    float night_vision_noise;

    /* CRT */
    bool  crt;
    float crt_curvature;

    /* Пьяная камера */
    bool  drunk;
    float drunk_amount;
} PostFX;

/* ================================================================
   СВЕТ
================================================================ */
#define MAX_LIGHTS 32

typedef struct {
    Vec2  pos;
    Color color;
    float radius;
    float intensity;
    bool  active;
    float flicker;
    float phase;
    float inner_radius;
    float spot_angle;
    bool  is_spot;
} Light;

/* ================================================================
   RENDERER
================================================================ */
typedef struct {
    SDL_Renderer* sdl_renderer;
    SDL_Texture*  stream_tex;
    Uint32        pixels[PB_W * PB_H];
    Uint32        tmp[PB_W * PB_H];

    Camera  camera;
    int     win_w, win_h;
    Color   clear_color;

    Light   lights[MAX_LIGHTS];
    int     light_count;
    bool    lighting_enabled;
    Color   ambient_light;
    Uint32  lightmap[PB_W * PB_H];

    PostFX  postfx;
    float   time;

    Rng     rng;
} Renderer;

/* ================================================================
   ИНИЦИАЛИЗАЦИЯ
================================================================ */
Renderer* renderer_create(SDL_Renderer* sdl_renderer);
void      renderer_destroy(Renderer* r);
void      renderer_resize(Renderer* r, int win_w, int win_h);
void      renderer_set_clear_color(Renderer* r, Color c);
void      renderer_set_bg_color(Renderer* r, Color c);

/* ================================================================
   КАМЕРА
================================================================ */
void renderer_set_camera(Renderer* r, Camera cam);
void renderer_move_camera(Renderer* r, Vec2 delta);
void renderer_zoom_camera(Renderer* r, float factor);
void renderer_set_camera_zoom(Renderer* r, float zoom);
void renderer_shake_camera(Renderer* r, float trauma);
void renderer_camera_follow(Renderer* r, Vec2 target, float dt);
void renderer_camera_update(Renderer* r, float dt);
Vec2 renderer_world_to_pixel(Renderer* r, Vec2 world);
Vec2 renderer_world_to_screen(Renderer* r, Vec2 world);
Vec2 renderer_screen_to_world(Renderer* r, Vec2 screen);

/* ================================================================
   ОЧИСТКА / КАДР
================================================================ */
void renderer_clear(Renderer* r);
void renderer_present(Renderer* r);

/* ================================================================
   ПИКСЕЛЬНЫЙ БУФЕР
================================================================ */
void  pb_put(Renderer* r, int x, int y, Color c);
void  pb_blend(Renderer* r, int x, int y, Color c);
void  pb_fill_rect(Renderer* r, int x, int y, int w, int h, Color c);
Color pb_get(Renderer* r, int x, int y);
void  pb_put_world(Renderer* r, Vec2 world, Color c);

/* ================================================================
   ОСВЕЩЕНИЕ
================================================================ */
void  renderer_apply_lighting(Renderer* r);
int   renderer_add_light(Renderer* r, Vec2 pos, Color color, float radius, float intensity);
void  renderer_remove_light(Renderer* r, int id);
void  renderer_move_light(Renderer* r, int id, Vec2 pos);
void  renderer_set_light_flicker(Renderer* r, int id, float flicker);
void  renderer_set_light_color(Renderer* r, int id, Color color);
void  renderer_set_light_radius(Renderer* r, int id, float radius);
Light* renderer_get_light(Renderer* r, int id);

/* ================================================================
   ПРИМИТИВЫ
================================================================ */
void renderer_draw_pixel(Renderer* r, Vec2 pos, Color c);

/* Линии */
void renderer_draw_line(Renderer* r, Vec2 a, Vec2 b, Color c);
void renderer_draw_line_aa(Renderer* r, Vec2 a, Vec2 b, Color c);
void renderer_draw_line_thick(Renderer* r, Vec2 a, Vec2 b, float width, Color c);

/* Прямоугольники */
void renderer_draw_rect(Renderer* r, Rect rect, Color c, bool filled);
void renderer_draw_rect_gradient(Renderer* r, Rect rect, Gradient g);
void renderer_draw_rect_rounded(Renderer* r, Rect rect, float radius, Color c, bool filled);

/* Окружности / эллипсы */
void renderer_draw_circle(Renderer* r, Vec2 center, float radius, Color c, bool filled);
void renderer_draw_circle_aa(Renderer* r, Vec2 center, float radius, Color c);
void renderer_draw_circle_outline(Renderer* r, Vec2 center, float radius, float thickness, Color c);
void renderer_draw_ellipse(Renderer* r, Vec2 center, float rx, float ry, Color c, bool filled);
void renderer_draw_ring(Renderer* r, Vec2 center, float inner, float outer, Color c);
void renderer_draw_arc(Renderer* r, Vec2 center, float radius, float a0, float a1, Color c);
void renderer_draw_sector(Renderer* r, Vec2 center, float radius, float a0, float a1, Color c);

/* Многоугольники */
void renderer_draw_triangle(Renderer* r, Vec2 a, Vec2 b, Vec2 c, Color col, bool filled);
void renderer_draw_triangle_gradient(Renderer* r, Vec2 a, Vec2 b, Vec2 c, Color ca, Color cb, Color cc);
void renderer_draw_polygon(Renderer* r, Vec2* verts, int n, Color c, bool filled);

/* Прочие формы */
void renderer_draw_capsule(Renderer* r, Vec2 a, Vec2 b, float radius, Color c);
void renderer_draw_arrow(Renderer* r, Vec2 from, Vec2 to, float head, Color c);
void renderer_draw_shadow(Renderer* r, Vec2 pos, float radius, float opacity);
void renderer_draw_aabb(Renderer* r, AABB b, Color c);
void renderer_draw_obb(Renderer* r, OBB o, Color c);

/* Кривые */
void renderer_draw_bezier(Renderer* r, Vec2 p0, Vec2 p1, Vec2 p2, int steps, Color c);
void renderer_draw_bezier3(Renderer* r, Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, int steps, Color c);
void renderer_draw_spline(Renderer* r, Vec2* points, int n, int steps_per_seg, Color c);

/* ================================================================
   ТЕКСТУРЫ
================================================================ */
Texture* renderer_load_texture(Renderer* r, const char* path);
void     renderer_draw_texture(Renderer* r, Texture* tex, Vec2 pos, float rot, float scale);
void     renderer_draw_texture_ex(Renderer* r, Texture* tex, Vec2 pos, float rot, Vec2 scale, Vec2 pivot, Color tint, bool flip_x, bool flip_y);
void     renderer_draw_texture_rect(Renderer* r, Texture* tex, TexRect src, Vec2 pos, float scale, Color tint);
void     renderer_destroy_texture(Texture* tex);

/* ================================================================
   ТЕКСТ (bitmap 5x7)
================================================================ */
void renderer_draw_text(Renderer* r, const char* text, Vec2 pos, Color c, float scale);
void renderer_draw_text_centered(Renderer* r, const char* text, Vec2 center, Color c, float scale);
Vec2 renderer_measure_text(const char* text, float scale);
void renderer_draw_int(Renderer* r, int val, Vec2 pos, Color c, float scale);
void renderer_draw_float(Renderer* r, float val, int decimals, Vec2 pos, Color c, float scale);

/* ================================================================
   ФИЗИКА → РЕНДЕР
================================================================ */
void renderer_draw_particle(Renderer* r, Particle* p);
void renderer_draw_spring(Renderer* r, Spring* s, Color c);
void renderer_draw_cloth(Renderer* r, Cloth* cloth);
void renderer_draw_softbody(Renderer* r, SoftBody* body);
void renderer_draw_trigger_zone(Renderer* r, TriggerZone* t, Color c);
void renderer_draw_force_field(Renderer* r, ForceField* ff);
void renderer_debug_particle(Renderer* r, Particle* p, bool show_vel, bool show_forces);
void renderer_debug_physics(Renderer* r, PhysicsWorld* w);
void renderer_debug_raycast(Renderer* r, Vec2 origin, Vec2 dir, float max_dist, RaycastHit* hit);

/* ================================================================
   ОГОНЬ
================================================================ */
#define MAX_FIRE_HOTSPOTS 16
typedef struct {
    float   buf[PB_W * PB_H];
    Color   palette[256];
    float   t;
    float   wind;
    float   intensity;
    int     hotspot_count;
    float   hotspot_x[MAX_FIRE_HOTSPOTS];
    float   hotspot_phase[MAX_FIRE_HOTSPOTS];
} FireSim;
FireSim* fire_create(int hotspots);
void     fire_destroy(FireSim* f);
void     fire_set_wind(FireSim* f, float w);
void     fire_set_intensity(FireSim* f, float i);
void     fire_add_hotspot(FireSim* f, float nx);
void     fire_update(FireSim* f, float dt);
void     fire_render(Renderer* r, FireSim* f, int px, int py, int pw, int ph);

/* ================================================================
   ДЫМ
================================================================ */
#define MAX_SMOKE_PARTS 256
typedef struct { float x,y,vx,vy,life,decay,size; } SmokeParticle;
typedef struct {
    SmokeParticle parts[MAX_SMOKE_PARTS];
    int   count;
    Color color;
} SmokeSim;
SmokeSim* smoke_create(void);
void      smoke_destroy(SmokeSim* s);
void      smoke_emit(SmokeSim* s, float x, float y, float wind);
void      smoke_emit_color(SmokeSim* s, float x, float y, float wind, Color c);
void      smoke_update(SmokeSim* s, float dt, float wind);
void      smoke_render(Renderer* r, SmokeSim* s);

/* ================================================================
   ДОЖДЬ
================================================================ */
#define MAX_RAIN_DROPS 400
typedef struct { float x,y,vy,len,vx,life; } RainDrop;
typedef struct {
    RainDrop drops[MAX_RAIN_DROPS];
    bool  active;
    float intensity;
    Color color;
} RainSim;
RainSim* rain_create(void);
void     rain_destroy(RainSim* rn);
void     rain_set_intensity(RainSim* rn, float i);
void     rain_update(RainSim* rn, float dt, float wind);
void     rain_render(Renderer* r, RainSim* rn, float wind);

/* ================================================================
   ИСКРЫ
================================================================ */
#define MAX_SPARK_PARTS 128
#define SPARK_TRAIL 12
typedef struct {
    float x,y,vx,vy,life,decay;
    float tx[SPARK_TRAIL],ty[SPARK_TRAIL];
    Color color;
} SparkParticle;
typedef struct { SparkParticle parts[MAX_SPARK_PARTS]; } SparkSim;
SparkSim* spark_create(void);
void      spark_destroy(SparkSim* s);
void      spark_emit(SparkSim* s, float x, float y, float wind);
void      spark_emit_color(SparkSim* s, float x, float y, float wind, Color c, float speed);
void      spark_update(SparkSim* s, float dt, float wind);
void      spark_render(Renderer* r, SparkSim* s);

/* ================================================================
   СНЕГ
================================================================ */
#define MAX_SNOW 256
typedef struct { float x,y,vx,vy,size,spin; } SnowFlake;
typedef struct {
    SnowFlake flakes[MAX_SNOW];
    int   count;
    float wind, gravity, density;
    bool  active;
    Color color;
} SnowSim;
SnowSim* snow_create(int count);
void     snow_destroy(SnowSim* s);
void     snow_set_wind(SnowSim* s, float wind);
void     snow_update(SnowSim* s, float dt);
void     snow_render(Renderer* r, SnowSim* s);

/* ================================================================
   МОЛНИЯ
================================================================ */
#define MAX_LIGHTNING     16
#define LIGHTNING_MAX_PTS 128
typedef struct {
    Vec2  points[LIGHTNING_MAX_PTS];
    int   count;
    float life, max_life;
    Color color, glow_color;
    float width, glow_width;
    int   branches;
} LightningSeg;
typedef struct {
    LightningSeg segs[MAX_LIGHTNING];
    int count;
} LightningSim;
LightningSim* lightning_create(void);
void          lightning_destroy(LightningSim* l);
void          lightning_strike(LightningSim* l, Vec2 from, Vec2 to, int branches);
void          lightning_update(LightningSim* l, float dt);
void          lightning_render(Renderer* r, LightningSim* l);

/* ================================================================
   RIPPLE
================================================================ */
#define MAX_RIPPLES 32
typedef struct { float x,y,radius,max_radius,life,speed,strength; Color color; } Ripple;
typedef struct { Ripple ripples[MAX_RIPPLES]; int count; } RippleSim;
RippleSim* ripple_create(void);
void       ripple_destroy(RippleSim* rs);
void       ripple_add(RippleSim* rs, Vec2 pos, float speed, float max_radius, Color c);
void       ripple_update(RippleSim* rs, float dt);
void       ripple_render(Renderer* r, RippleSim* rs);

/* ================================================================
   METABALL
================================================================ */
#define MAX_METABALLS 16
typedef struct { float x,y,radius; Color color; } Metaball;
typedef struct {
    Metaball balls[MAX_METABALLS];
    int   count;
    float threshold;
    bool  show_edge;
    Color fill_color, edge_color;
} MetaballSim;
MetaballSim* metaball_create(void);
void         metaball_destroy(MetaballSim* ms);
void         metaball_add(MetaballSim* ms, Vec2 pos, float radius, Color c);
void         metaball_set_position(MetaballSim* ms, int i, Vec2 pos);
void         metaball_update(MetaballSim* ms, float dt);
void         metaball_render(Renderer* r, MetaballSim* ms);

/* ================================================================
   УНИВЕРСАЛЬНАЯ СИСТЕМА ЧАСТИЦ
================================================================ */
#define PS_MAX_PARTICLES 1024

typedef enum { PS_EMITTER_POINT=0, PS_EMITTER_CIRCLE, PS_EMITTER_RECT, PS_EMITTER_LINE, PS_EMITTER_DIRECTION } PSEmitterShape;
typedef enum { PS_RENDER_CIRCLE=0, PS_RENDER_SQUARE, PS_RENDER_CROSS, PS_RENDER_STAR } PSRenderMode;

typedef struct {
    float x,y,vx,vy,life,inv_life,size,rotation,ang_vel;
    Color color, color_end;
    bool  active;
} PSParticle;

typedef struct {
    PSParticle    particles[PS_MAX_PARTICLES];
    int           max_particles;
    float         ex, ey;
    float         emit_rate, emit_accumulator;
    float         speed_min, speed_max;
    float         life_min,  life_max;
    float         size_min,  size_max;
    float         ang_vel_min, ang_vel_max;
    Color         color_start_min, color_start_max;
    Color         color_end_min,   color_end_max;
    float         gravity_scale;
    float         drag;
    float         size_over_life;
    PSEmitterShape emitter_shape;
    float          emitter_radius;
    float          emitter_w, emitter_h;
    float          emitter_spread;
    Vec2           emitter_direction;
    PSRenderMode   render_mode;
    bool  looping;
    bool  world_space;
    bool  paused;
    float duration, elapsed;
} ParticleSystem;

ParticleSystem* ps_create(void);
void  ps_destroy(ParticleSystem* ps);
void  ps_set_position(ParticleSystem* ps, float x, float y);
void  ps_emit(ParticleSystem* ps, int count);
void  ps_burst(ParticleSystem* ps, int count);
void  ps_update(ParticleSystem* ps, float dt);
void  ps_render(Renderer* r, ParticleSystem* ps);
void  ps_stop(ParticleSystem* ps);
void  ps_play(ParticleSystem* ps);
bool  ps_is_alive(ParticleSystem* ps);

/* ================================================================
   TRAIL
================================================================ */
#define TRAIL_MAX_POINTS 128

typedef struct {
    Vec2  points[TRAIL_MAX_POINTS];
    Color colors[TRAIL_MAX_POINTS];
    float ages[TRAIL_MAX_POINTS];
    float widths[TRAIL_MAX_POINTS];
    int   head, count;
    float point_lifetime;
    float min_distance;
    float width_start, width_end;
    Color color_start, color_end;
    bool  fade;
} Trail;

Trail* trail_create(float lifetime, float min_dist);
void   trail_destroy(Trail* t);
void   trail_clear(Trail* t);
void   trail_add_point(Trail* t, Vec2 pos, Color c);
void   trail_update(Trail* t, float dt);
void   trail_render(Renderer* r, Trail* t);

/* ================================================================
   TILEMAP
================================================================ */
#define TILEMAP_MAX_LAYERS 8

typedef struct {
    Texture* tileset;
    int      tileset_cols;
    int*     data;
    int      width, height;
    bool     visible;
    float    parallax_x, parallax_y;
    Color    tint;
} TilemapLayer;

typedef struct {
    TilemapLayer layers[TILEMAP_MAX_LAYERS];
    int  layer_count;
    int  tile_w, tile_h;
} Tilemap;

Tilemap*     tilemap_create(int tile_w, int tile_h);
void         tilemap_destroy(Tilemap* tm);
TilemapLayer* tilemap_add_layer(Tilemap* tm, Texture* tileset, int cols, int map_w, int map_h);
void         tilemap_set_tile(TilemapLayer* l, int x, int y, int id);
int          tilemap_get_tile(TilemapLayer* l, int x, int y);
void         tilemap_render(Renderer* r, Tilemap* tm);
void         tilemap_render_layer(Renderer* r, Tilemap* tm, int layer_idx);

/* ================================================================
   ANIMATED SPRITE
================================================================ */
#define ANIM_MAX_CLIPS  16
#define ANIM_MAX_FRAMES 64

typedef struct {
    char   name[32];
    TexRect frames[ANIM_MAX_FRAMES];
    int    frame_count;
    float  fps;
    bool   loop;
} AnimClip;

typedef struct {
    Texture*  sheet;
    AnimClip  clips[ANIM_MAX_CLIPS];
    int       clip_count;
    int       current_clip;
    int       current_frame;
    float     frame_timer;
    bool      playing, finished;
    float     scale;
    Vec2      pivot;
    Color     tint;
    float     rotation;
    bool      flip_x, flip_y;
    void      (*on_frame)(int frame, void* ud);
    void      (*on_end)(void* ud);
    void*     user_data;
} AnimatedSprite;

AnimatedSprite* anim_sprite_create(Texture* sheet);
void   anim_sprite_destroy(AnimatedSprite* s);
int    anim_sprite_add_clip(AnimatedSprite* s, const char* name, int sx, int sy, int fw, int fh, int frame_count, float fps, bool loop);
void   anim_sprite_play(AnimatedSprite* s, int clip_index);
void   anim_sprite_play_name(AnimatedSprite* s, const char* name);
void   anim_sprite_stop(AnimatedSprite* s);
void   anim_sprite_update(AnimatedSprite* s, float dt);
void   anim_sprite_render(Renderer* r, AnimatedSprite* s, Vec2 pos);

#endif /* RENDER_H */
