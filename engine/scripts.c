/* scripts.c — Cngine v2
 * LGPL v3 | Chikatilo / sooborn / SGLauncher
 */
#include "scripts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

/* ================================================================
   ВНУТРЕННИЕ УТИЛИТЫ
================================================================ */
static void _lua_error(lua_State* L, const char* context){
    printf("[Scripts] Error in %s: %s\n", context, lua_tostring(L,-1));
    lua_pop(L,1);
}

static bool _call_func(lua_State* L, const char* name, int nargs, int nret){
    lua_getglobal(L, name);
    if(!lua_isfunction(L,-1)){
        lua_pop(L,1); return false;
    }
    /* сдвинуть аргументы поверх функции (аргументы уже на стеке) */
    if(lua_pcall(L, nargs, nret, 0)!=LUA_OK){
        _lua_error(L, name);
        return false;
    }
    return true;
}

static long _file_mtime(const char* path){
    struct stat st;
    if(stat(path,&st)!=0) return 0;
    return (long)st.st_mtime;
}

/* ================================================================
   БИНДИНГИ: ENGINE / CORE
================================================================ */
static ScriptManager* _sm_ptr = NULL; /* глобальный указатель для биндингов */

static int l_engine_quit(lua_State* L){
    (void)L;
    if(_sm_ptr&&_sm_ptr->game) game_quit(_sm_ptr->game);
    return 0;
}
static int l_engine_pause(lua_State* L){
    bool p=lua_toboolean(L,1);
    if(_sm_ptr&&_sm_ptr->game){
        if(p) game_pause(_sm_ptr->game); else game_resume(_sm_ptr->game);
    }
    return 0;
}
static int l_engine_set_time_scale(lua_State* L){
    float s=(float)luaL_checknumber(L,1);
    if(_sm_ptr&&_sm_ptr->game) game_set_time_scale(_sm_ptr->game,s);
    return 0;
}
static int l_engine_dt(lua_State* L){
    float dt=(_sm_ptr&&_sm_ptr->game)?game_get_delta_time(_sm_ptr->game):0.016f;
    lua_pushnumber(L,dt); return 1;
}
static int l_engine_fps(lua_State* L){
    int fps=(_sm_ptr&&_sm_ptr->game)?game_get_fps(_sm_ptr->game):0;
    lua_pushinteger(L,fps); return 1;
}
static int l_engine_elapsed(lua_State* L){
    float e=(_sm_ptr&&_sm_ptr->game)?game_get_elapsed(_sm_ptr->game):0.f;
    lua_pushnumber(L,e); return 1;
}
static int l_engine_width(lua_State* L){
    int w=(_sm_ptr&&_sm_ptr->game)?_sm_ptr->game->window_width:0;
    lua_pushinteger(L,w); return 1;
}
static int l_engine_height(lua_State* L){
    int h=(_sm_ptr&&_sm_ptr->game)?_sm_ptr->game->window_height:0;
    lua_pushinteger(L,h); return 1;
}
static int l_engine_mouse_x(lua_State* L){
    float x=(_sm_ptr&&_sm_ptr->game)?(float)_sm_ptr->game->input.mouse_x:0.f;
    lua_pushnumber(L,x); return 1;
}
static int l_engine_mouse_y(lua_State* L){
    float y=(_sm_ptr&&_sm_ptr->game)?(float)_sm_ptr->game->input.mouse_y:0.f;
    lua_pushnumber(L,y); return 1;
}
static int l_engine_key_down(lua_State* L){
    int k=luaL_checkinteger(L,1);
    bool r=(_sm_ptr&&_sm_ptr->game&&k<MAX_KEYS)?_sm_ptr->game->input.keys[k]:false;
    lua_pushboolean(L,r); return 1;
}
static int l_engine_key_pressed(lua_State* L){
    int k=luaL_checkinteger(L,1);
    bool r=(_sm_ptr&&_sm_ptr->game&&k<MAX_KEYS)?_sm_ptr->game->input.keys_pressed[k]:false;
    lua_pushboolean(L,r); return 1;
}

static const luaL_Reg _engine_lib[] = {
    {"quit",           l_engine_quit},
    {"pause",          l_engine_pause},
    {"set_time_scale", l_engine_set_time_scale},
    {"dt",             l_engine_dt},
    {"fps",            l_engine_fps},
    {"elapsed",        l_engine_elapsed},
    {"width",          l_engine_width},
    {"height",         l_engine_height},
    {"mouse_x",        l_engine_mouse_x},
    {"mouse_y",        l_engine_mouse_y},
    {"key_down",       l_engine_key_down},
    {"key_pressed",    l_engine_key_pressed},
    {NULL, NULL}
};

/* ================================================================
   БИНДИНГИ: RENDER
================================================================ */
static Renderer* _get_renderer(void){
    if(!_sm_ptr||!_sm_ptr->game) return NULL;
    return _sm_ptr->game->renderer;
}
static Color _l_color(lua_State* L, int ri, int gi, int bi, int ai){
    Uint8 r=(Uint8)luaL_optinteger(L,ri,255);
    Uint8 g=(Uint8)luaL_optinteger(L,gi,255);
    Uint8 b=(Uint8)luaL_optinteger(L,bi,255);
    Uint8 a=(Uint8)luaL_optinteger(L,ai,255);
    return color_rgba(r,g,b,a);
}
static int l_render_clear(lua_State* L){ (void)L; Renderer*r=_get_renderer(); if(r)renderer_clear(r); return 0; }
static int l_render_draw_rect(lua_State* L){
    float x=(float)luaL_checknumber(L,1), y=(float)luaL_checknumber(L,2);
    float w=(float)luaL_checknumber(L,3), h=(float)luaL_checknumber(L,4);
    Color c=_l_color(L,5,6,7,8);
    bool filled=lua_toboolean(L,9);
    Renderer*rr=_get_renderer(); if(rr) renderer_draw_rect(rr,rect(x,y,w,h),c,filled);
    return 0;
}
static int l_render_draw_circle(lua_State* L){
    float x=(float)luaL_checknumber(L,1), y=(float)luaL_checknumber(L,2);
    float rad=(float)luaL_checknumber(L,3);
    Color c=_l_color(L,4,5,6,7);
    bool filled=lua_toboolean(L,8);
    Renderer*rr=_get_renderer(); if(rr) renderer_draw_circle(rr,vec2(x,y),rad,c,filled);
    return 0;
}
static int l_render_draw_line(lua_State* L){
    float x1=(float)luaL_checknumber(L,1),y1=(float)luaL_checknumber(L,2);
    float x2=(float)luaL_checknumber(L,3),y2=(float)luaL_checknumber(L,4);
    Color c=_l_color(L,5,6,7,8);
    Renderer*rr=_get_renderer(); if(rr) renderer_draw_line(rr,vec2(x1,y1),vec2(x2,y2),c);
    return 0;
}
static int l_render_draw_text(lua_State* L){
    const char* txt=luaL_checkstring(L,1);
    float x=(float)luaL_checknumber(L,2), y=(float)luaL_checknumber(L,3);
    Color c=_l_color(L,4,5,6,7);
    float scale=(float)luaL_optnumber(L,8,1.0);
    Renderer*rr=_get_renderer(); if(rr) renderer_draw_text(rr,txt,vec2(x,y),c,scale);
    return 0;
}
static int l_render_set_clear_color(lua_State* L){
    Color c=_l_color(L,1,2,3,4);
    Renderer*rr=_get_renderer(); if(rr) renderer_set_clear_color(rr,c);
    return 0;
}
static int l_render_camera_set(lua_State* L){
    float x=(float)luaL_checknumber(L,1), y=(float)luaL_checknumber(L,2);
    Renderer*rr=_get_renderer();
    if(rr){ rr->camera.position=vec2(x,y); }
    return 0;
}
static int l_render_camera_zoom(lua_State* L){
    float f=(float)luaL_checknumber(L,1);
    Renderer*rr=_get_renderer(); if(rr) renderer_zoom_camera(rr,f);
    return 0;
}
static int l_render_camera_shake(lua_State* L){
    float t=(float)luaL_checknumber(L,1);
    Renderer*rr=_get_renderer(); if(rr) renderer_shake_camera(rr,t);
    return 0;
}
/* PostFX */
static int l_postfx_bloom(lua_State* L){
    bool on=lua_toboolean(L,1);
    int rad=luaL_optinteger(L,2,4);
    float str=(float)luaL_optnumber(L,3,1.5f);
    float thr=(float)luaL_optnumber(L,4,0.7f);
    Renderer*rr=_get_renderer();
    if(rr){rr->postfx.bloom=on;rr->postfx.bloom_radius=rad;rr->postfx.bloom_strength=str;rr->postfx.bloom_threshold=thr;}
    return 0;
}
static int l_postfx_vignette(lua_State* L){
    bool on=lua_toboolean(L,1);
    float str=(float)luaL_optnumber(L,2,0.5f);
    Renderer*rr=_get_renderer();
    if(rr){rr->postfx.vignette=on;rr->postfx.vignette_strength=str;}
    return 0;
}
static int l_postfx_grain(lua_State* L){
    bool on=lua_toboolean(L,1);
    float str=(float)luaL_optnumber(L,2,0.05f);
    Renderer*rr=_get_renderer();
    if(rr){rr->postfx.film_grain=on;rr->postfx.grain_strength=str;}
    return 0;
}
static int l_postfx_preset(lua_State* L){
    const char* name=luaL_checkstring(L,1);
    Renderer*rr=_get_renderer();
    if(!rr) return 0;
    /* минимальный inline пресет */
    memset(&rr->postfx,0,sizeof(PostFX));
    rr->postfx.saturation=1.f; rr->postfx.contrast=1.f;
    rr->postfx.brightness=1.f; rr->postfx.gamma=1.f;
    if(strcmp(name,"retro")==0){
        rr->postfx.scanlines=true; rr->postfx.scanlines_alpha=0.4f; rr->postfx.scanlines_gap=2;
        rr->postfx.pixelate=true; rr->postfx.pixel_size=3;
        rr->postfx.chromatic_aberration=true; rr->postfx.ca_strength=1.5f;
    } else if(strcmp(name,"cinematic")==0){
        rr->postfx.vignette=true; rr->postfx.vignette_strength=0.6f;
        rr->postfx.film_grain=true; rr->postfx.grain_strength=0.03f;
        rr->postfx.color_grade=true; rr->postfx.saturation=0.9f; rr->postfx.contrast=1.2f;
    } else if(strcmp(name,"neon")==0){
        rr->postfx.bloom=true; rr->postfx.bloom_radius=5; rr->postfx.bloom_strength=2.5f;
        rr->postfx.color_grade=true; rr->postfx.saturation=1.4f; rr->postfx.contrast=1.3f;
    } else if(strcmp(name,"horror")==0){
        rr->postfx.vignette=true; rr->postfx.vignette_strength=0.85f;
        rr->postfx.film_grain=true; rr->postfx.grain_strength=0.12f;
        rr->postfx.color_grade=true; rr->postfx.saturation=0.1f;
        rr->postfx.drunk=true; rr->postfx.drunk_amount=0.008f;
    }
    return 0;
}

static const luaL_Reg _render_lib[] = {
    {"clear",          l_render_clear},
    {"draw_rect",      l_render_draw_rect},
    {"draw_circle",    l_render_draw_circle},
    {"draw_line",      l_render_draw_line},
    {"draw_text",      l_render_draw_text},
    {"set_clear_color",l_render_set_clear_color},
    {"camera_set",     l_render_camera_set},
    {"camera_zoom",    l_render_camera_zoom},
    {"camera_shake",   l_render_camera_shake},
    {"postfx_bloom",   l_postfx_bloom},
    {"postfx_vignette",l_postfx_vignette},
    {"postfx_grain",   l_postfx_grain},
    {"postfx_preset",  l_postfx_preset},
    {NULL,NULL}
};

/* ================================================================
   БИНДИНГИ: AUDIO
================================================================ */
static int l_audio_load(lua_State* L){
    const char* path=luaL_checkstring(L,1);
    const char* name=luaL_checkstring(L,2);
    if(_sm_ptr&&_sm_ptr->audio) audio_load_sound(_sm_ptr->audio,path,name);
    return 0;
}
static int l_audio_play(lua_State* L){
    const char* name=luaL_checkstring(L,1);
    float vol=(float)luaL_optnumber(L,2,1.0);
    float pit=(float)luaL_optnumber(L,3,1.0);
    float pan=(float)luaL_optnumber(L,4,0.0);
    int ci=(_sm_ptr&&_sm_ptr->audio)?audio_play(_sm_ptr->audio,name,vol,pit,pan):-1;
    lua_pushinteger(L,ci); return 1;
}
static int l_audio_play_at(lua_State* L){
    const char* name=luaL_checkstring(L,1);
    float x=(float)luaL_checknumber(L,2), y=(float)luaL_checknumber(L,3);
    float vol=(float)luaL_optnumber(L,4,1.0);
    int ci=(_sm_ptr&&_sm_ptr->audio)?audio_play_at(_sm_ptr->audio,name,x,y,vol):-1;
    lua_pushinteger(L,ci); return 1;
}
static int l_audio_play_music(lua_State* L){
    const char* name=luaL_checkstring(L,1);
    float vol=(float)luaL_optnumber(L,2,1.0);
    bool loop=lua_toboolean(L,3);
    if(_sm_ptr&&_sm_ptr->audio) audio_play_music(_sm_ptr->audio,name,vol,loop);
    return 0;
}
static int l_audio_stop_music(lua_State* L){
    (void)L;
    if(_sm_ptr&&_sm_ptr->audio) audio_stop_music(_sm_ptr->audio);
    return 0;
}
static int l_audio_crossfade(lua_State* L){
    const char* name=luaL_checkstring(L,1);
    float secs=(float)luaL_optnumber(L,2,1.0);
    if(_sm_ptr&&_sm_ptr->audio) audio_crossfade_music(_sm_ptr->audio,name,secs);
    return 0;
}
static int l_audio_trigger(lua_State* L){
    const char* name=luaL_checkstring(L,1);
    int ci=(_sm_ptr&&_sm_ptr->audio)?audio_trigger(_sm_ptr->audio,name):-1;
    lua_pushinteger(L,ci); return 1;
}
static int l_audio_set_master(lua_State* L){
    float v=(float)luaL_checknumber(L,1);
    if(_sm_ptr&&_sm_ptr->audio) audio_set_master_volume(_sm_ptr->audio,v);
    return 0;
}
static int l_audio_set_music_vol(lua_State* L){
    float v=(float)luaL_checknumber(L,1);
    if(_sm_ptr&&_sm_ptr->audio) audio_set_music_volume(_sm_ptr->audio,v);
    return 0;
}
static int l_audio_set_sfx_vol(lua_State* L){
    float v=(float)luaL_checknumber(L,1);
    if(_sm_ptr&&_sm_ptr->audio) audio_set_sfx_volume(_sm_ptr->audio,v);
    return 0;
}
static int l_audio_register_event(lua_State* L){
    const char* trig=luaL_checkstring(L,1);
    const char* snd =luaL_checkstring(L,2);
    float vmin=(float)luaL_optnumber(L,3,0.8);
    float vmax=(float)luaL_optnumber(L,4,1.0);
    float pmin=(float)luaL_optnumber(L,5,0.9);
    float pmax=(float)luaL_optnumber(L,6,1.1);
    float cd  =(float)luaL_optnumber(L,7,0.05);
    if(_sm_ptr&&_sm_ptr->audio)
        audio_register_event(_sm_ptr->audio,trig,snd,vmin,vmax,pmin,pmax,cd);
    return 0;
}

static const luaL_Reg _audio_lib[] = {
    {"load",            l_audio_load},
    {"play",            l_audio_play},
    {"play_at",         l_audio_play_at},
    {"play_music",      l_audio_play_music},
    {"stop_music",      l_audio_stop_music},
    {"crossfade",       l_audio_crossfade},
    {"trigger",         l_audio_trigger},
    {"set_master",      l_audio_set_master},
    {"set_music_vol",   l_audio_set_music_vol},
    {"set_sfx_vol",     l_audio_set_sfx_vol},
    {"register_event",  l_audio_register_event},
    {NULL,NULL}
};

/* ================================================================
   БИНДИНГИ: PHYSICS
================================================================ */
static int l_phys_set_gravity(lua_State* L){
    float x=(float)luaL_checknumber(L,1), y=(float)luaL_checknumber(L,2);
    if(_sm_ptr&&_sm_ptr->game&&_sm_ptr->game->physics_world)
        physics_world_set_gravity(_sm_ptr->game->physics_world, vec2(x,y));
    return 0;
}
static int l_phys_explosion(lua_State* L){
    float x=(float)luaL_checknumber(L,1), y=(float)luaL_checknumber(L,2);
    float r=(float)luaL_checknumber(L,3), f=(float)luaL_checknumber(L,4);
    if(_sm_ptr&&_sm_ptr->game&&_sm_ptr->game->physics_world)
        physics_explosion(_sm_ptr->game->physics_world,vec2(x,y),r,f);
    return 0;
}
static int l_phys_raycast(lua_State* L){
    float ox=(float)luaL_checknumber(L,1),oy=(float)luaL_checknumber(L,2);
    float dx=(float)luaL_checknumber(L,3),dy=(float)luaL_checknumber(L,4);
    float dist=(float)luaL_optnumber(L,5,1000.0);
    if(!_sm_ptr||!_sm_ptr->game||!_sm_ptr->game->physics_world){
        lua_pushboolean(L,false); return 1;
    }
    RaycastHit h=physics_raycast(_sm_ptr->game->physics_world,vec2(ox,oy),vec2(dx,dy),dist,LAYER_ALL);
    lua_pushboolean(L,h.hit);
    lua_pushnumber(L,h.point.x); lua_pushnumber(L,h.point.y);
    lua_pushnumber(L,h.normal.x); lua_pushnumber(L,h.normal.y);
    lua_pushnumber(L,h.distance);
    return 6;
}

static const luaL_Reg _phys_lib[] = {
    {"set_gravity", l_phys_set_gravity},
    {"explosion",   l_phys_explosion},
    {"raycast",     l_phys_raycast},
    {NULL,NULL}
};

/* ================================================================
   РЕГИСТРАЦИЯ БИНДИНГОВ
================================================================ */
static void _register_libs(lua_State* L){
    luaL_newlib(L, _engine_lib); lua_setglobal(L, "engine");
    luaL_newlib(L, _render_lib); lua_setglobal(L, "render");
    luaL_newlib(L, _audio_lib);  lua_setglobal(L, "audio");
    luaL_newlib(L, _phys_lib);   lua_setglobal(L, "phys");

    /* Константы клавиш (SDL scancode) */
    lua_newtable(L);
    lua_pushinteger(L, SDL_SCANCODE_W);     lua_setfield(L,-2,"W");
    lua_pushinteger(L, SDL_SCANCODE_A);     lua_setfield(L,-2,"A");
    lua_pushinteger(L, SDL_SCANCODE_S);     lua_setfield(L,-2,"S");
    lua_pushinteger(L, SDL_SCANCODE_D);     lua_setfield(L,-2,"D");
    lua_pushinteger(L, SDL_SCANCODE_SPACE); lua_setfield(L,-2,"SPACE");
    lua_pushinteger(L, SDL_SCANCODE_ESCAPE);lua_setfield(L,-2,"ESCAPE");
    lua_pushinteger(L, SDL_SCANCODE_RETURN);lua_setfield(L,-2,"RETURN");
    lua_pushinteger(L, SDL_SCANCODE_LEFT);  lua_setfield(L,-2,"LEFT");
    lua_pushinteger(L, SDL_SCANCODE_RIGHT); lua_setfield(L,-2,"RIGHT");
    lua_pushinteger(L, SDL_SCANCODE_UP);    lua_setfield(L,-2,"UP");
    lua_pushinteger(L, SDL_SCANCODE_DOWN);  lua_setfield(L,-2,"DOWN");
    lua_setglobal(L,"Key");
}

/* ================================================================
   СОЗДАНИЕ / УНИЧТОЖЕНИЕ
================================================================ */
ScriptManager* scripts_create(GameState* game, AudioSystem* audio, HitboxManager* hbm){
    ScriptManager* sm=(ScriptManager*)calloc(1,sizeof(ScriptManager));
    sm->game=game; sm->audio=audio; sm->hbm=hbm;
    sm->watch_interval=1.f;

    _sm_ptr=sm;

    sm->global_L=luaL_newstate();
    luaL_openlibs(sm->global_L);
    _register_libs(sm->global_L);

    printf("[Scripts] Initialized (Lua %s)\n", LUA_VERSION);
    return sm;
}

void scripts_destroy(ScriptManager* sm){
    if(!sm) return;
    for(int i=0;i<sm->count;i++){
        if(sm->scripts[i].loaded && sm->scripts[i].L){
            scripts_call_destroy(sm,i);
            lua_close(sm->scripts[i].L);
        }
    }
    if(sm->global_L) lua_close(sm->global_L);
    free(sm);
    _sm_ptr=NULL;
}

/* ================================================================
   ЗАГРУЗКА
================================================================ */
static lua_State* _make_state(void){
    lua_State* L=luaL_newstate();
    luaL_openlibs(L);
    _register_libs(L);
    return L;
}

int scripts_load(ScriptManager* sm, const char* path, const char* name){
    if(!sm||sm->count>=SCRIPTS_MAX) return -1;
    Script* s=&sm->scripts[sm->count];
    memset(s,0,sizeof(Script));
    strncpy(s->path,path,255);
    strncpy(s->name,name,63);

    s->L=_make_state();
    if(luaL_dofile(s->L,path)!=LUA_OK){
        printf("[Scripts] Failed to load '%s': %s\n",path,lua_tostring(s->L,-1));
        lua_pop(s->L,1);
        lua_close(s->L);
        return -1;
    }
    /* Проверить наличие callback-функций */
    lua_getglobal(s->L,"on_init");    s->has_init   =lua_isfunction(s->L,-1); lua_pop(s->L,1);
    lua_getglobal(s->L,"on_update");  s->has_update =lua_isfunction(s->L,-1); lua_pop(s->L,1);
    lua_getglobal(s->L,"on_render");  s->has_render =lua_isfunction(s->L,-1); lua_pop(s->L,1);
    lua_getglobal(s->L,"on_event");   s->has_event  =lua_isfunction(s->L,-1); lua_pop(s->L,1);
    lua_getglobal(s->L,"on_destroy"); s->has_destroy=lua_isfunction(s->L,-1); lua_pop(s->L,1);

    s->loaded=true;
    s->last_mtime=_file_mtime(path);
    printf("[Scripts] Loaded '%s' (%s)\n",name,path);
    return sm->count++;
}

bool scripts_reload(ScriptManager* sm, int idx){
    if(!sm||idx<0||idx>=sm->count) return false;
    Script* s=&sm->scripts[idx];
    if(!s->loaded) return false;
    /* Вызвать on_destroy */
    scripts_call_destroy(sm,idx);
    lua_close(s->L);
    s->L=_make_state();
    if(luaL_dofile(s->L,s->path)!=LUA_OK){
        printf("[Scripts] Reload failed '%s': %s\n",s->name,lua_tostring(s->L,-1));
        lua_pop(s->L,1);
        return false;
    }
    lua_getglobal(s->L,"on_init");    s->has_init   =lua_isfunction(s->L,-1); lua_pop(s->L,1);
    lua_getglobal(s->L,"on_update");  s->has_update =lua_isfunction(s->L,-1); lua_pop(s->L,1);
    lua_getglobal(s->L,"on_render");  s->has_render =lua_isfunction(s->L,-1); lua_pop(s->L,1);
    lua_getglobal(s->L,"on_event");   s->has_event  =lua_isfunction(s->L,-1); lua_pop(s->L,1);
    lua_getglobal(s->L,"on_destroy"); s->has_destroy=lua_isfunction(s->L,-1); lua_pop(s->L,1);
    s->last_mtime=_file_mtime(s->path);
    printf("[Scripts] Reloaded '%s'\n",s->name);
    scripts_call_init(sm,idx);
    return true;
}

void scripts_watch(ScriptManager* sm, int idx, bool on){
    if(!sm||idx<0||idx>=sm->count) return;
    sm->scripts[idx].watch=on;
}

void scripts_unload(ScriptManager* sm, int idx){
    if(!sm||idx<0||idx>=sm->count) return;
    Script* s=&sm->scripts[idx];
    if(s->L){ scripts_call_destroy(sm,idx); lua_close(s->L); s->L=NULL; }
    s->loaded=false;
}

int scripts_find(ScriptManager* sm, const char* name){
    for(int i=0;i<sm->count;i++)
        if(sm->scripts[i].loaded && strcmp(sm->scripts[i].name,name)==0) return i;
    return -1;
}

/* ================================================================
   ВЫЗОВЫ
================================================================ */
void scripts_call_init(ScriptManager* sm, int idx){
    if(!sm||idx<0||idx>=sm->count) return;
    Script* s=&sm->scripts[idx];
    if(!s->loaded||!s->has_init) return;
    _call_func(s->L,"on_init",0,0);
}

void scripts_call_update(ScriptManager* sm, int idx, float dt){
    if(!sm||idx<0||idx>=sm->count) return;
    Script* s=&sm->scripts[idx];
    if(!s->loaded||!s->has_update) return;
    lua_getglobal(s->L,"on_update");
    lua_pushnumber(s->L,dt);
    if(lua_pcall(s->L,1,0,0)!=LUA_OK) _lua_error(s->L,"on_update");
}

void scripts_call_render(ScriptManager* sm, int idx){
    if(!sm||idx<0||idx>=sm->count) return;
    Script* s=&sm->scripts[idx];
    if(!s->loaded||!s->has_render) return;
    _call_func(s->L,"on_render",0,0);
}

void scripts_call_event(ScriptManager* sm, int idx, const char* ev){
    if(!sm||idx<0||idx>=sm->count) return;
    Script* s=&sm->scripts[idx];
    if(!s->loaded||!s->has_event) return;
    lua_getglobal(s->L,"on_event");
    lua_pushstring(s->L,ev);
    if(lua_pcall(s->L,1,0,0)!=LUA_OK) _lua_error(s->L,"on_event");
}

void scripts_call_destroy(ScriptManager* sm, int idx){
    if(!sm||idx<0||idx>=sm->count) return;
    Script* s=&sm->scripts[idx];
    if(!s->loaded||!s->has_destroy) return;
    _call_func(s->L,"on_destroy",0,0);
}

void scripts_update_all(ScriptManager* sm, float dt){
    if(!sm) return;
    for(int i=0;i<sm->count;i++) scripts_call_update(sm,i,dt);
}
void scripts_render_all(ScriptManager* sm){
    if(!sm) return;
    for(int i=0;i<sm->count;i++) scripts_call_render(sm,i);
}
void scripts_init_all(ScriptManager* sm){
    if(!sm) return;
    for(int i=0;i<sm->count;i++) scripts_call_init(sm,i);
}
void scripts_destroy_all(ScriptManager* sm){
    if(!sm) return;
    for(int i=0;i<sm->count;i++) scripts_call_destroy(sm,i);
}

void scripts_check_watches(ScriptManager* sm, float dt){
    if(!sm) return;
    sm->watch_timer+=dt;
    if(sm->watch_timer<sm->watch_interval) return;
    sm->watch_timer=0.f;
    for(int i=0;i<sm->count;i++){
        Script* s=&sm->scripts[i];
        if(!s->loaded||!s->watch) continue;
        long mt=_file_mtime(s->path);
        if(mt!=s->last_mtime){
            printf("[Scripts] File changed, reloading '%s'\n",s->name);
            scripts_reload(sm,i);
        }
    }
}

/* ================================================================
   НИЗКОУРОВНЕВЫЙ ДОСТУП
================================================================ */
lua_State* scripts_get_state(ScriptManager* sm, int idx){
    if(!sm||idx<0||idx>=sm->count) return NULL;
    return sm->scripts[idx].L;
}
lua_State* scripts_global_state(ScriptManager* sm){ return sm?sm->global_L:NULL; }

bool scripts_exec_string(ScriptManager* sm, int idx, const char* code){
    lua_State* L=scripts_get_state(sm,idx);
    if(!L) return false;
    if(luaL_dostring(L,code)!=LUA_OK){ _lua_error(L,"exec_string"); return false; }
    return true;
}
bool scripts_exec_global(ScriptManager* sm, const char* code){
    if(!sm||!sm->global_L) return false;
    if(luaL_dostring(sm->global_L,code)!=LUA_OK){ _lua_error(sm->global_L,"exec_global"); return false; }
    return true;
}

void scripts_set_global_int(ScriptManager* sm, int idx, const char* name, int val){
    lua_State* L=scripts_get_state(sm,idx); if(!L) return;
    lua_pushinteger(L,val); lua_setglobal(L,name);
}
void scripts_set_global_float(ScriptManager* sm, int idx, const char* name, float val){
    lua_State* L=scripts_get_state(sm,idx); if(!L) return;
    lua_pushnumber(L,val); lua_setglobal(L,name);
}
void scripts_set_global_string(ScriptManager* sm, int idx, const char* name, const char* val){
    lua_State* L=scripts_get_state(sm,idx); if(!L) return;
    lua_pushstring(L,val); lua_setglobal(L,name);
}
int scripts_get_global_int(ScriptManager* sm, int idx, const char* name){
    lua_State* L=scripts_get_state(sm,idx); if(!L) return 0;
    lua_getglobal(L,name); int v=(int)lua_tointeger(L,-1); lua_pop(L,1); return v;
}
float scripts_get_global_float(ScriptManager* sm, int idx, const char* name){
    lua_State* L=scripts_get_state(sm,idx); if(!L) return 0.f;
    lua_getglobal(L,name); float v=(float)lua_tonumber(L,-1); lua_pop(L,1); return v;
}
