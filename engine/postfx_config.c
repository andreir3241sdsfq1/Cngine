/* postfx_config.c — Cngine v2
 * LGPL v3 | Chikatilo / sooborn / SGLauncher
 */
#include "postfx_config.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

/* ================================================================
   ВКЛЮЧЕНИЕ / ВЫКЛЮЧЕНИЕ
================================================================ */
void postfx_enable_bloom(Renderer* r, bool on, int radius, float strength, float threshold){
#ifdef CNGINE_NO_POSTFX
    (void)r;(void)on;(void)radius;(void)strength;(void)threshold; return;
#endif
#ifdef CNGINE_NO_BLOOM
    return;
#endif
    if(!r) return;
    r->postfx.bloom=on;
    if(on){ r->postfx.bloom_radius=radius; r->postfx.bloom_strength=strength; r->postfx.bloom_threshold=threshold; }
}

void postfx_enable_vignette(Renderer* r, bool on, float strength, Color color){
#if defined(CNGINE_NO_POSTFX)||defined(CNGINE_NO_VIGNETTE)
    (void)r;(void)on;(void)strength;(void)color; return;
#endif
    if(!r) return;
    r->postfx.vignette=on;
    if(on){ r->postfx.vignette_strength=strength; r->postfx.vignette_color=color; }
}

void postfx_enable_chromatic(Renderer* r, bool on, float strength){
#if defined(CNGINE_NO_POSTFX)||defined(CNGINE_NO_CHROMAB)
    (void)r;(void)on;(void)strength; return;
#endif
    if(!r) return;
    r->postfx.chromatic_aberration=on;
    if(on) r->postfx.ca_strength=strength;
}

void postfx_enable_scanlines(Renderer* r, bool on, float alpha, int gap){
#if defined(CNGINE_NO_POSTFX)||defined(CNGINE_NO_SCANLINES)
    (void)r;(void)on;(void)alpha;(void)gap; return;
#endif
    if(!r) return;
    r->postfx.scanlines=on;
    if(on){ r->postfx.scanlines_alpha=alpha; r->postfx.scanlines_gap=gap; }
}

void postfx_enable_grain(Renderer* r, bool on, float strength){
#if defined(CNGINE_NO_POSTFX)||defined(CNGINE_NO_FILMGRAIN)
    (void)r;(void)on;(void)strength; return;
#endif
    if(!r) return;
    r->postfx.film_grain=on;
    if(on){ r->postfx.grain_strength=strength; r->postfx.film_grain_strength=strength; }
}

void postfx_enable_colorgrade(Renderer* r, bool on,
                               float sat, float con, float bri, float gam, Color tint){
#if defined(CNGINE_NO_POSTFX)||defined(CNGINE_NO_COLORGRADE)
    (void)r;(void)on;(void)sat;(void)con;(void)bri;(void)gam;(void)tint; return;
#endif
    if(!r) return;
    r->postfx.color_grade=on;
    if(on){ r->postfx.saturation=sat; r->postfx.contrast=con;
            r->postfx.brightness=bri; r->postfx.gamma=gam; r->postfx.color_tint=tint; }
}

void postfx_enable_crt(Renderer* r, bool on, float curvature){
#if defined(CNGINE_NO_POSTFX)||defined(CNGINE_NO_CRT)
    (void)r;(void)on;(void)curvature; return;
#endif
    if(!r) return;
    r->postfx.crt=on;
    if(on) r->postfx.crt_curvature=curvature;
}

void postfx_enable_heat(Renderer* r, bool on, float intensity, float speed){
#if defined(CNGINE_NO_POSTFX)||defined(CNGINE_NO_HEATWAVE)
    (void)r;(void)on;(void)intensity;(void)speed; return;
#endif
    if(!r) return;
    r->postfx.heat_haze=on;
    if(on){ r->postfx.heat_intensity=intensity; r->postfx.heat_speed=speed; }
}

void postfx_enable_underwater(Renderer* r, bool on, float distort, Color tint){
#if defined(CNGINE_NO_POSTFX)||defined(CNGINE_NO_UNDERWATER)
    (void)r;(void)on;(void)distort;(void)tint; return;
#endif
    if(!r) return;
    r->postfx.underwater=on;
    if(on){ r->postfx.underwater_distort=distort; r->postfx.underwater_tint=tint; }
}

void postfx_enable_nightvision(Renderer* r, bool on, float noise){
#if defined(CNGINE_NO_POSTFX)||defined(CNGINE_NO_NIGHTVISION)
    (void)r;(void)on;(void)noise; return;
#endif
    if(!r) return;
    r->postfx.night_vision=on;
    if(on) r->postfx.night_vision_noise=noise;
}

void postfx_enable_drunk(Renderer* r, bool on, float amount){
#if defined(CNGINE_NO_POSTFX)||defined(CNGINE_NO_DRUNK)
    (void)r;(void)on;(void)amount; return;
#endif
    if(!r) return;
    r->postfx.drunk=on;
    if(on) r->postfx.drunk_amount=amount;
}

void postfx_enable_pixelate(Renderer* r, bool on, int pixel_size){
#ifdef CNGINE_NO_POSTFX
    (void)r;(void)on;(void)pixel_size; return;
#endif
    if(!r) return;
    r->postfx.pixelate=on;
    if(on) r->postfx.pixel_size=pixel_size;
}

void postfx_disable_all(Renderer* r){
    if(!r) return;
    memset(&r->postfx, 0, sizeof(PostFX));
    /* Восстановить безопасные дефолты */
    r->postfx.saturation=1.f;
    r->postfx.contrast=1.f;
    r->postfx.brightness=1.f;
    r->postfx.gamma=1.f;
}

/* ================================================================
   ПРЕСЕТЫ
================================================================ */
void postfx_apply_preset(Renderer* r, PostFXPreset preset){
    if(!r) return;
    postfx_disable_all(r);

    Color white = {255,255,255,255};
    Color black = {0,0,0,255};
    Color green_night = {30,200,60,255};
    Color cyan_water  = {20,120,200,200};

    switch(preset){
        case PFX_PRESET_NONE:
            break;

        case PFX_PRESET_RETRO:
            postfx_enable_scanlines(r, true, 0.4f, 2);
            postfx_enable_pixelate(r, true, 3);
            postfx_enable_chromatic(r, true, 1.5f);
            postfx_enable_colorgrade(r, true, 0.8f, 1.1f, 0.95f, 1.0f, white);
            break;

        case PFX_PRESET_CINEMATIC:
            postfx_enable_vignette(r, true, 0.6f, black);
            postfx_enable_colorgrade(r, true, 0.9f, 1.2f, 0.9f, 1.1f, white);
            postfx_enable_grain(r, true, 0.03f);
            break;

        case PFX_PRESET_HORROR:
            postfx_enable_vignette(r, true, 0.85f, black);
            postfx_enable_grain(r, true, 0.12f);
            postfx_enable_colorgrade(r, true, 0.1f, 1.3f, 0.7f, 0.9f, white);
            postfx_enable_drunk(r, true, 0.008f);
            break;

        case PFX_PRESET_UNDERWATER:
            postfx_enable_underwater(r, true, 0.04f, cyan_water);
            postfx_enable_chromatic(r, true, 1.0f);
            postfx_enable_colorgrade(r, true, 0.8f, 1.0f, 0.7f, 1.0f, cyan_water);
            break;

        case PFX_PRESET_NIGHT:
            postfx_enable_nightvision(r, true, 0.05f);
            postfx_enable_vignette(r, true, 0.5f, black);
            break;

        case PFX_PRESET_NEON:
            postfx_enable_bloom(r, true, 5, 2.5f, 0.6f);
            postfx_enable_colorgrade(r, true, 1.4f, 1.3f, 1.1f, 1.0f, white);
            postfx_enable_chromatic(r, true, 0.8f);
            break;

        case PFX_PRESET_DREAM:
            postfx_enable_bloom(r, true, 8, 1.5f, 0.5f);
            postfx_enable_chromatic(r, true, 2.0f);
            postfx_enable_drunk(r, true, 0.005f);
            postfx_enable_colorgrade(r, true, 1.1f, 0.9f, 1.1f, 1.0f, white);
            break;

        default: break;
    }
}

/* ================================================================
   УМНЫЙ BLOOM
================================================================ */
void postfx_smart_bloom(Renderer* r, int radius, float strength, bool auto_bright){
#if defined(CNGINE_NO_POSTFX)||defined(CNGINE_NO_BLOOM)
    (void)r;(void)radius;(void)strength;(void)auto_bright; return;
#endif
    if(!r||radius<=0) return;

    /* Создаём маску: какие пиксели будут bloom */
    static float bloom_mask[PB_W*PB_H];
    static float bloom_buf[PB_W*PB_H*3]; /* RGB float */

    /* Заполнить маску */
    for(int i=0;i<PB_W*PB_H;i++){
        Uint32 px=r->pixels[i];
        Uint8 a=(Uint8)(px&0xFF);
        Uint8 rr=(Uint8)((px>>24)&0xFF);
        Uint8 g=(Uint8)((px>>16)&0xFF);
        Uint8 b=(Uint8)((px>>8)&0xFF);

        bool tagged = (a==BLOOM_ALPHA_TAG);
        bool bright = false;
        if(auto_bright){
            float lum=(rr*0.2126f+g*0.7152f+b*0.0722f)/255.f;
            bright=(lum>SMART_BLOOM_AUTO_THRESHOLD);
        }
        if(tagged||bright){
            bloom_mask[i]=1.f;
            bloom_buf[i*3+0]=(float)rr/255.f;
            bloom_buf[i*3+1]=(float)g/255.f;
            bloom_buf[i*3+2]=(float)b/255.f;
        } else {
            bloom_mask[i]=0.f;
            bloom_buf[i*3+0]=bloom_buf[i*3+1]=bloom_buf[i*3+2]=0.f;
        }
    }

    /* Горизонтальный blur маски */
    static float hblur[PB_W*PB_H*3];
    for(int y=0;y<PB_H;y++){
        for(int x=0;x<PB_W;x++){
            float sr=0,sg=0,sb=0,sw=0;
            for(int kx=-radius;kx<=radius;kx++){
                int nx=x+kx;
                if(nx<0||nx>=PB_W) continue;
                float w=1.f - fabsf((float)kx/radius);
                int idx2=(y*PB_W+nx)*3;
                sr+=bloom_buf[idx2+0]*w;
                sg+=bloom_buf[idx2+1]*w;
                sb+=bloom_buf[idx2+2]*w;
                sw+=w;
            }
            if(sw>0){ sr/=sw; sg/=sw; sb/=sw; }
            int idx=(y*PB_W+x)*3;
            hblur[idx+0]=sr; hblur[idx+1]=sg; hblur[idx+2]=sb;
        }
    }

    /* Вертикальный blur + добавить в пиксельный буфер */
    for(int y=0;y<PB_H;y++){
        for(int x=0;x<PB_W;x++){
            float sr=0,sg=0,sb=0,sw=0;
            for(int ky=-radius;ky<=radius;ky++){
                int ny=y+ky;
                if(ny<0||ny>=PB_H) continue;
                float w=1.f - fabsf((float)ky/radius);
                int idx2=(ny*PB_W+x)*3;
                sr+=hblur[idx2+0]*w;
                sg+=hblur[idx2+1]*w;
                sb+=hblur[idx2+2]*w;
                sw+=w;
            }
            if(sw>0){ sr/=sw; sg/=sw; sb/=sw; }
            /* Добавить bloom к существующему пикселю */
            int idx=y*PB_W+x;
            Uint32 px=r->pixels[idx];
            int orr=(int)((px>>24)&0xFF);
            int og=(int)((px>>16)&0xFF);
            int ob=(int)((px>>8)&0xFF);
            int oa=(int)(px&0xFF);
            int nr=orr+(int)(sr*strength*255.f);
            int ng=og+(int)(sg*strength*255.f);
            int nb=ob+(int)(sb*strength*255.f);
            if(nr>255)nr=255; if(ng>255)ng=255; if(nb>255)nb=255;
            r->pixels[idx]=((Uint32)nr<<24)|((Uint32)ng<<16)|((Uint32)nb<<8)|(Uint32)oa;
        }
    }
}

/* ================================================================
   АНИМАТОР ПОСТОБРАБОТКИ
================================================================ */
PostFXAnimator* postfx_animator_create(Renderer* r){
    PostFXAnimator* a=(PostFXAnimator*)calloc(1,sizeof(PostFXAnimator));
    if(r){ a->current=r->postfx; a->target=r->postfx; }
    a->blend_speed=1.f;
    return a;
}

void postfx_animator_destroy(PostFXAnimator* a){ free(a); }

void postfx_animator_set_target(PostFXAnimator* a, PostFXPreset preset){
    if(!a) return;
    /* Создать временный renderer для получения пресета */
    PostFX tmp={0};
    tmp.saturation=1.f; tmp.contrast=1.f; tmp.brightness=1.f; tmp.gamma=1.f;
    /* Применяем пресет вручную без рендерера */
    switch(preset){
        case PFX_PRESET_CINEMATIC:
            tmp.vignette=true; tmp.vignette_strength=0.6f;
            tmp.color_grade=true; tmp.saturation=0.9f; tmp.contrast=1.2f; tmp.brightness=0.9f;
            tmp.film_grain=true; tmp.grain_strength=0.03f;
            break;
        case PFX_PRESET_HORROR:
            tmp.vignette=true; tmp.vignette_strength=0.85f;
            tmp.film_grain=true; tmp.grain_strength=0.12f;
            tmp.color_grade=true; tmp.saturation=0.1f; tmp.contrast=1.3f;
            tmp.drunk=true; tmp.drunk_amount=0.008f;
            break;
        default: break;
    }
    a->target=tmp;
    a->blend=0.f;
    a->transitioning=true;
}

void postfx_animator_update(PostFXAnimator* a, Renderer* r, float dt){
    if(!a||!r||!a->transitioning) return;
    a->blend+=dt*a->blend_speed;
    if(a->blend>=1.f){ a->blend=1.f; a->transitioning=false; }
    float t=a->blend;
    /* Линейная интерполяция числовых полей */
    PostFX* cur=&a->current;
    PostFX* tgt=&a->target;
#define LERP_FIELD(f) r->postfx.f = cur->f + (tgt->f - cur->f)*t
    LERP_FIELD(bloom_strength); LERP_FIELD(vignette_strength); LERP_FIELD(ca_strength);
    LERP_FIELD(grain_strength); LERP_FIELD(saturation); LERP_FIELD(contrast);
    LERP_FIELD(brightness); LERP_FIELD(gamma); LERP_FIELD(drunk_amount);
    LERP_FIELD(heat_intensity); LERP_FIELD(underwater_distort);
    LERP_FIELD(night_vision_noise); LERP_FIELD(crt_curvature);
#undef LERP_FIELD
    /* Булевы: переключаем по mid-point */
#define SWITCH_BOOL(f) r->postfx.f = (t>=0.5f)?tgt->f:cur->f
    SWITCH_BOOL(bloom); SWITCH_BOOL(vignette); SWITCH_BOOL(chromatic_aberration);
    SWITCH_BOOL(film_grain); SWITCH_BOOL(color_grade); SWITCH_BOOL(crt);
    SWITCH_BOOL(heat_haze); SWITCH_BOOL(underwater); SWITCH_BOOL(night_vision);
    SWITCH_BOOL(drunk); SWITCH_BOOL(scanlines);
#undef SWITCH_BOOL
}
