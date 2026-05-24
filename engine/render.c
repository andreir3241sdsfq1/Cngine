/* render.c — Cngine v2
 * LGPL v3 | Chikatilo / sooborn / SGLauncher */
#include "render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ======================== ЦВЕТА ======================== */
const Color COLOR_WHITE       = {255,255,255,255};
const Color COLOR_BLACK       = {  0,  0,  0,255};
const Color COLOR_RED         = {255,  0,  0,255};
const Color COLOR_GREEN       = {  0,255,  0,255};
const Color COLOR_BLUE        = {  0,  0,255,255};
const Color COLOR_YELLOW      = {255,255,  0,255};
const Color COLOR_MAGENTA     = {255,  0,255,255};
const Color COLOR_CYAN        = {  0,255,255,255};
const Color COLOR_GRAY        = {128,128,128,255};
const Color COLOR_ORANGE      = {255,140,  0,255};
const Color COLOR_DARKGRAY    = { 64, 64, 64,255};
const Color COLOR_TRANSPARENT = {  0,  0,  0,  0};
const Color COLOR_PURPLE      = {128,  0,128,255};
const Color COLOR_TEAL        = {  0,128,128,255};
const Color COLOR_LIME        = {  0,255,  0,255};
const Color COLOR_PINK        = {255,105,180,255};
const Color COLOR_BROWN       = {139, 69, 19,255};
const Color COLOR_NAVY        = {  0,  0,128,255};
const Color COLOR_MAROON      = {128,  0,  0,255};
const Color COLOR_OLIVE       = {128,128,  0,255};

/* ======================== ПИКСЕЛЬНЫЙ БУФЕР ======================== */
static inline Uint32 _pack(Uint8 r,Uint8 g,Uint8 b){
    return ((Uint32)r<<16)|((Uint32)g<<8)|b;
}

void pb_put(Renderer* r,int x,int y,Color c){
    if((unsigned)x>=(unsigned)PB_W||(unsigned)y>=(unsigned)PB_H)return;
    r->pixels[y*PB_W+x]=_pack(c.r,c.g,c.b);
}

void pb_blend(Renderer* r,int x,int y,Color c){
    if((unsigned)x>=(unsigned)PB_W||(unsigned)y>=(unsigned)PB_H)return;
    if(c.a==0)return;
    if(c.a==255){r->pixels[y*PB_W+x]=_pack(c.r,c.g,c.b);return;}
    Uint32 bg=r->pixels[y*PB_W+x]; int ia=255-c.a;
    int nr=((bg>>16)&0xFF)*ia/255+c.r*c.a/255;
    int ng=((bg>> 8)&0xFF)*ia/255+c.g*c.a/255;
    int nb=((bg    )&0xFF)*ia/255+c.b*c.a/255;
    r->pixels[y*PB_W+x]=_pack(nr,ng,nb);
}

void pb_fill_rect(Renderer* r,int x,int y,int w,int h,Color c){
    for(int dy=0;dy<h;dy++) for(int dx=0;dx<w;dx++) pb_blend(r,x+dx,y+dy,c);
}

Color pb_get(Renderer* r,int x,int y){
    if((unsigned)x>=(unsigned)PB_W||(unsigned)y>=(unsigned)PB_H)return COLOR_BLACK;
    Uint32 p=r->pixels[y*PB_W+x];
    return color_rgb((p>>16)&0xFF,(p>>8)&0xFF,p&0xFF);
}

void pb_put_world(Renderer* r,Vec2 world,Color c){
    Vec2 px=renderer_world_to_pixel(r,world); pb_blend(r,(int)px.x,(int)px.y,c);
}

/* ======================== ИНИЦИАЛИЗАЦИЯ ======================== */
Renderer* renderer_create(SDL_Renderer* sdl_renderer){
    Renderer* r=(Renderer*)calloc(1,sizeof(Renderer));
    if(!r)return NULL;
    r->sdl_renderer=sdl_renderer;
    r->stream_tex=SDL_CreateTexture(sdl_renderer,SDL_PIXELFORMAT_RGB888,SDL_TEXTUREACCESS_STREAMING,PB_W,PB_H);
    r->camera.zoom=1.f;
    r->camera.position=vec2(0,0);
    r->camera.trauma_decay=2.5f;
    r->camera.smooth_speed=8.f;
    r->clear_color=COLOR_BLACK;
    r->ambient_light=color_rgb(30,30,30);
    r->lighting_enabled=false;
    r->postfx.bloom_strength=0.06f;
    r->postfx.bloom_radius=2;
    r->postfx.bloom_threshold=0.4f;
    r->postfx.vignette_strength=0.5f;
    r->postfx.saturation=1.f;
    r->postfx.contrast=1.f;
    r->postfx.grain_strength=0.1f;
    r->postfx.scanlines_gap=2;
    r->postfx.scanlines_strength=0.3f;
    r->postfx.pixel_size=2;
    r->win_w=960; r->win_h=600;
    r->rng.state=0xDEADBEEF12345678ULL;
    return r;
}

void renderer_destroy(Renderer* r){
    if(!r)return;
    if(r->stream_tex)SDL_DestroyTexture(r->stream_tex);
    free(r);
}

void renderer_resize(Renderer* r,int w,int h){ if(r){r->win_w=w;r->win_h=h;} }
void renderer_set_clear_color(Renderer* r,Color c){ if(r)r->clear_color=c; }
void renderer_set_bg_color(Renderer* r,Color c){ if(r)r->clear_color=c; }

/* ======================== КАМЕРА ======================== */
void renderer_set_camera(Renderer* r,Camera cam){ r->camera=cam; }
void renderer_move_camera(Renderer* r,Vec2 delta){ r->camera.position=vec2_add(r->camera.position,delta); }
void renderer_zoom_camera(Renderer* r,float factor){ r->camera.zoom*=factor; }
void renderer_set_camera_zoom(Renderer* r,float zoom){ r->camera.zoom=zoom; }

void renderer_shake_camera(Renderer* r,float trauma){
    r->camera.trauma+=trauma;
    if(r->camera.trauma>1.f)r->camera.trauma=1.f;
}

void renderer_camera_follow(Renderer* r,Vec2 target,float dt){
    float spd=r->camera.smooth_speed>0?r->camera.smooth_speed:10.f;
    r->camera.position.x+=(target.x-r->camera.position.x)*spd*dt;
    r->camera.position.y+=(target.y-r->camera.position.y)*spd*dt;
}

void renderer_camera_update(Renderer* r,float dt){
    float decay=r->camera.trauma_decay>0?r->camera.trauma_decay:2.5f;
    r->camera.trauma-=decay*dt;
    if(r->camera.trauma<0)r->camera.trauma=0;
    float s=r->camera.trauma*r->camera.trauma;
    r->camera.shake_x=(rng_float(&r->rng)*2.f-1.f)*15.f*s;
    r->camera.shake_y=(rng_float(&r->rng)*2.f-1.f)*10.f*s;
}

Vec2 renderer_world_to_pixel(Renderer* r,Vec2 world){
    float dx=(world.x-r->camera.position.x)*r->camera.zoom;
    float dy=(world.y-r->camera.position.y)*r->camera.zoom;
    return vec2(PB_W*.5f+dx+r->camera.shake_x, PB_H*.5f+dy+r->camera.shake_y);
}

Vec2 renderer_world_to_screen(Renderer* r,Vec2 world){
    Vec2 pp=renderer_world_to_pixel(r,world);
    return vec2(pp.x*(float)r->win_w/PB_W, pp.y*(float)r->win_h/PB_H);
}

Vec2 renderer_screen_to_world(Renderer* r,Vec2 screen){
    float px=screen.x*PB_W/(float)r->win_w;
    float py=screen.y*PB_H/(float)r->win_h;
    return vec2((px-PB_W*.5f-r->camera.shake_x)/r->camera.zoom+r->camera.position.x,
                (py-PB_H*.5f-r->camera.shake_y)/r->camera.zoom+r->camera.position.y);
}

/* ======================== ОЧИСТКА ======================== */
void renderer_clear(Renderer* r){
    Uint32 bg=_pack(r->clear_color.r,r->clear_color.g,r->clear_color.b);
    for(int i=0;i<PB_W*PB_H;i++) r->pixels[i]=bg;
    memset(r->lightmap,0,sizeof(r->lightmap));
    r->time+=0.016f;
}

/* ======================== ОСВЕЩЕНИЕ ======================== */
void renderer_apply_lighting(Renderer* r){
    if(!r->lighting_enabled)return;
    for(int i=0;i<r->light_count;i++){
        Light* l=&r->lights[i]; if(!l->active)continue;
        float flick=1.f;
        if(l->flicker>0)flick=1.f-(rng_float(&r->rng)*l->flicker);
        float rad=l->radius*r->camera.zoom*flick;
        Vec2 lp=renderer_world_to_pixel(r,l->pos);
        int x0=(int)(lp.x-rad),x1=(int)(lp.x+rad);
        int y0=(int)(lp.y-rad),y1=(int)(lp.y+rad);
        if(x0<0)x0=0; if(x1>=PB_W)x1=PB_W-1;
        if(y0<0)y0=0; if(y1>=PB_H)y1=PB_H-1;
        for(int y=y0;y<=y1;y++) for(int x=x0;x<=x1;x++){
            float dx=x-lp.x,dy=y-lp.y,d2=dx*dx+dy*dy;
            if(d2>rad*rad)continue;
            float att=1.f-(sqrtf(d2)/rad); att=att*att;
            float br=att*l->intensity*flick;
            Uint32 lv=r->lightmap[y*PB_W+x];
            int lr=(int)((lv>>16)&0xFF)+(int)(l->color.r*br);
            int lg=(int)((lv>>8)&0xFF) +(int)(l->color.g*br);
            int lb=(int)(lv&0xFF)      +(int)(l->color.b*br);
            if(lr>255)lr=255; if(lg>255)lg=255; if(lb>255)lb=255;
            r->lightmap[y*PB_W+x]=_pack((Uint8)lr,(Uint8)lg,(Uint8)lb);
        }
    }
    for(int i=0;i<PB_W*PB_H;i++){
        Uint32 p=r->pixels[i]; Uint32 lv=r->lightmap[i];
        int ar=r->ambient_light.r,ag=r->ambient_light.g,ab=r->ambient_light.b;
        int lr=(lv>>16)&0xFF,lg=(lv>>8)&0xFF,lb=lv&0xFF;
        int pr=(p>>16)&0xFF,pg=(p>>8)&0xFF,pb=(p)&0xFF;
        int fr=(pr*(lr+ar))/255; if(fr>255)fr=255;
        int fg=(pg*(lg+ag))/255; if(fg>255)fg=255;
        int fb=(pb*(lb+ab))/255; if(fb>255)fb=255;
        r->pixels[i]=_pack((Uint8)fr,(Uint8)fg,(Uint8)fb);
    }
}

int renderer_add_light(Renderer* r,Vec2 pos,Color color,float radius,float intensity){
    for(int i=0;i<MAX_LIGHTS;i++) if(!r->lights[i].active){
        r->lights[i].pos=pos; r->lights[i].color=color;
        r->lights[i].radius=radius; r->lights[i].intensity=intensity;
        r->lights[i].active=true;
        if(i>=r->light_count)r->light_count=i+1;
        return i;
    }
    return -1;
}
void   renderer_remove_light(Renderer* r,int id){ if(id>=0&&id<MAX_LIGHTS)r->lights[id].active=false; }
void   renderer_move_light(Renderer* r,int id,Vec2 pos){ if(id>=0&&id<MAX_LIGHTS)r->lights[id].pos=pos; }
void   renderer_set_light_flicker(Renderer* r,int id,float f){ if(id>=0&&id<MAX_LIGHTS)r->lights[id].flicker=f; }
void   renderer_set_light_color(Renderer* r,int id,Color c){ if(id>=0&&id<MAX_LIGHTS)r->lights[id].color=c; }
void   renderer_set_light_radius(Renderer* r,int id,float rad){ if(id>=0&&id<MAX_LIGHTS)r->lights[id].radius=rad; }
Light* renderer_get_light(Renderer* r,int id){ if(id>=0&&id<MAX_LIGHTS)return &r->lights[id]; return NULL; }

/* ======================== POSTFX ======================== */
static void _pf_bloom(Renderer* r){
    float thr=r->postfx.bloom_threshold*255.f;
    float str=r->postfx.bloom_strength;
    memcpy(r->tmp,r->pixels,sizeof(Uint32)*PB_W*PB_H);
    Uint32 h[PB_W*PB_H];
    for(int y=0;y<PB_H;y++) for(int x=0;x<PB_W;x++){
        int ra=0,ga=0,ba=0,n=0;
        for(int d=-4;d<=4;d++){int nx=x+d;if(nx<0||nx>=PB_W)continue;
            Uint32 p=r->tmp[y*PB_W+nx];
            float lum=(float)(((p>>16)&0xFF)*77+((p>>8)&0xFF)*150+(p&0xFF)*29)/256.f;
            if(lum>thr){ra+=(p>>16)&0xFF;ga+=(p>>8)&0xFF;ba+=p&0xFF;n++;}}
        h[y*PB_W+x]=n?_pack(ra/n,ga/n,ba/n):0;
    }
    for(int y=0;y<PB_H;y++) for(int x=0;x<PB_W;x++){
        int ra=0,ga=0,ba=0,n=0;
        for(int d=-4;d<=4;d++){int ny=y+d;if(ny<0||ny>=PB_H)continue;
            Uint32 p=h[ny*PB_W+x];if(!p)continue;ra+=(p>>16)&0xFF;ga+=(p>>8)&0xFF;ba+=p&0xFF;n++;}
        if(!n)continue;
        int pr=(r->pixels[y*PB_W+x]>>16)&0xFF,pg=(r->pixels[y*PB_W+x]>>8)&0xFF,pb=(r->pixels[y*PB_W+x])&0xFF;
        int nr=pr+(int)(ra/n*str),ng=pg+(int)(ga/n*str),nb=pb+(int)(ba/n*str);
        if(nr>255)nr=255; if(ng>255)ng=255; if(nb>255)nb=255;
        r->pixels[y*PB_W+x]=_pack((Uint8)nr,(Uint8)ng,(Uint8)nb);
    }
}

static void _pf_vignette(Renderer* r){
    float str=r->postfx.vignette_strength;
    float cx=PB_W*.5f,cy=PB_H*.5f,mx=sqrtf(cx*cx+cy*cy);
    for(int y=0;y<PB_H;y++) for(int x=0;x<PB_W;x++){
        float dx=x-cx,dy=y-cy,d=sqrtf(dx*dx+dy*dy)/mx;
        float v=1.f-d*d*str; if(v<0.f)v=0.f; if(v>1.f)v=1.f;
        Uint32 p=r->pixels[y*PB_W+x];
        r->pixels[y*PB_W+x]=_pack((Uint8)(((p>>16)&0xFF)*v),(Uint8)(((p>>8)&0xFF)*v),(Uint8)((p&0xFF)*v));
    }
}

static void _pf_chromatic(Renderer* r){
    int sh=(int)(r->postfx.ca_strength>0?r->postfx.ca_strength*3:2);
    memcpy(r->tmp,r->pixels,sizeof(Uint32)*PB_W*PB_H);
    for(int y=0;y<PB_H;y++) for(int x=0;x<PB_W;x++){
        int xr=x+sh; if(xr>=PB_W)xr=PB_W-1;
        int xb=x-sh; if(xb<0)xb=0;
        r->pixels[y*PB_W+x]=_pack((r->tmp[y*PB_W+xr]>>16)&0xFF,(r->tmp[y*PB_W+x]>>8)&0xFF,r->tmp[y*PB_W+xb]&0xFF);
    }
}

static void _pf_scanlines(Renderer* r){
    float str=r->postfx.scanlines_strength;
    int gap=r->postfx.scanlines_gap; if(gap<2)gap=2;
    for(int y=0;y<PB_H;y+=gap){
        for(int x=0;x<PB_W;x++){
            Uint32 p=r->pixels[y*PB_W+x];
            float f=1.f-str;
            r->pixels[y*PB_W+x]=_pack((Uint8)(((p>>16)&0xFF)*f),(Uint8)(((p>>8)&0xFF)*f),(Uint8)((p&0xFF)*f));
        }
    }
}

static void _pf_grain(Renderer* r){
    float str=r->postfx.film_grain_strength>0?r->postfx.film_grain_strength:r->postfx.grain_strength;
    for(int i=0;i<PB_W*PB_H;i++){
        int n=(int)((rng_float(&r->rng)*2.f-1.f)*str*80.f);
        Uint32 p=r->pixels[i];
        int rv=(int)((p>>16)&0xFF)+n, gv=(int)((p>>8)&0xFF)+n, bv=(int)(p&0xFF)+n;
        if(rv<0)rv=0; if(rv>255)rv=255;
        if(gv<0)gv=0; if(gv>255)gv=255;
        if(bv<0)bv=0; if(bv>255)bv=255;
        r->pixels[i]=_pack((Uint8)rv,(Uint8)gv,(Uint8)bv);
    }
}

static void _pf_pixelate(Renderer* r){
    int sz=r->postfx.pixel_size; if(sz<2)sz=2;
    for(int y=0;y<PB_H;y+=sz) for(int x=0;x<PB_W;x+=sz){
        Uint32 p=r->pixels[y*PB_W+x];
        for(int dy=0;dy<sz&&y+dy<PB_H;dy++) for(int dx=0;dx<sz&&x+dx<PB_W;dx++) r->pixels[(y+dy)*PB_W+x+dx]=p;
    }
}

static void _pf_heat_haze(Renderer* r){
    memcpy(r->tmp,r->pixels,sizeof(Uint32)*PB_W*PB_H);
    for(int y=0;y<PB_H;y++) for(int x=0;x<PB_W;x++){
        float wave=sinf(x*.08f+r->time*2.2f)*3.f+sinf(y*.06f+r->time*1.5f)*2.f;
        int sx=x+(int)wave; int sy=y+(int)(cosf(x*.05f+r->time)*1.5f);
        if(sx<0)sx=0; if(sx>=PB_W)sx=PB_W-1;
        if(sy<0)sy=0; if(sy>=PB_H)sy=PB_H-1;
        r->pixels[y*PB_W+x]=r->tmp[sy*PB_W+sx];
    }
}

static void _pf_underwater(Renderer* r){
    memcpy(r->tmp,r->pixels,sizeof(Uint32)*PB_W*PB_H);
    for(int y=0;y<PB_H;y++) for(int x=0;x<PB_W;x++){
        float wave=sinf(x*.04f+r->time*1.2f)*4.f;
        int sy=(int)(y+wave); if(sy<0)sy=0; if(sy>=PB_H)sy=PB_H-1;
        Uint32 p=r->tmp[sy*PB_W+x];
        int rv=(int)(((p>>16)&0xFF)*.6f);
        int gv=(int)(((p>>8)&0xFF)*.85f);
        int bv=(int)((p&0xFF));
        if(bv>255)bv=255;
        r->pixels[y*PB_W+x]=_pack((Uint8)rv,(Uint8)gv,(Uint8)bv);
    }
}

static void _pf_night_vision(Renderer* r){
    for(int i=0;i<PB_W*PB_H;i++){
        Uint32 p=r->pixels[i];
        float lum=(((p>>16)&0xFF)*.299f+((p>>8)&0xFF)*.587f+(p&0xFF)*.114f)/255.f;
        float n=(rng_float(&r->rng)*2.f-1.f)*0.06f;
        lum=lum+n; if(lum<0)lum=0; if(lum>1)lum=1;
        r->pixels[i]=_pack(0,(Uint8)(lum*200),0);
    }
}

void renderer_present(Renderer* r){
    if(r->postfx.pixelate)             _pf_pixelate(r);
    if(r->postfx.heat_haze)            _pf_heat_haze(r);
    if(r->postfx.underwater)           _pf_underwater(r);
    renderer_apply_lighting(r);
    if(r->postfx.bloom)                _pf_bloom(r);
    if(r->postfx.chromatic_aberration) _pf_chromatic(r);
    if(r->postfx.vignette)             _pf_vignette(r);
    if(r->postfx.scanlines)            _pf_scanlines(r);
    if(r->postfx.film_grain||r->postfx.grain_strength>0) _pf_grain(r);
    if(r->postfx.night_vision)         _pf_night_vision(r);
    SDL_UpdateTexture(r->stream_tex,NULL,r->pixels,PB_W*sizeof(Uint32));
    SDL_RenderClear(r->sdl_renderer);
    SDL_RenderCopy(r->sdl_renderer,r->stream_tex,NULL,NULL);
    SDL_RenderPresent(r->sdl_renderer);
}

/* ======================== ПРИМИТИВЫ ======================== */
void renderer_draw_pixel(Renderer* r,Vec2 pos,Color c){
    Vec2 p=renderer_world_to_pixel(r,pos); pb_blend(r,(int)p.x,(int)p.y,c);
}

void renderer_draw_line(Renderer* r,Vec2 a,Vec2 b,Color c){
    Vec2 pa=renderer_world_to_pixel(r,a),pb=renderer_world_to_pixel(r,b);
    int x0=(int)pa.x,y0=(int)pa.y,x1=(int)pb.x,y1=(int)pb.y;
    int dx=abs(x1-x0),dy=abs(y1-y0),sx=x0<x1?1:-1,sy=y0<y1?1:-1,err=dx-dy;
    for(;;){pb_blend(r,x0,y0,c);if(x0==x1&&y0==y1)break;int e2=2*err;if(e2>-dy){err-=dy;x0+=sx;}if(e2<dx){err+=dx;y0+=sy;}}
}

void renderer_draw_line_aa(Renderer* r,Vec2 a,Vec2 b,Color c){
    Vec2 pa=renderer_world_to_pixel(r,a),pb=renderer_world_to_pixel(r,b);
    float dx=pb.x-pa.x,dy=pb.y-pa.y;
    int steps=(int)(fabsf(dx)>fabsf(dy)?fabsf(dx):fabsf(dy))+1;
    for(int i=0;i<=steps;i++){
        float t=(float)i/steps;
        pb_blend(r,(int)(pa.x+dx*t),(int)(pa.y+dy*t),c);
    }
}

void renderer_draw_line_thick(Renderer* r,Vec2 a,Vec2 b,float width,Color c){
    renderer_draw_capsule(r,a,b,width*.5f,c);
}

void renderer_draw_rect(Renderer* r,Rect rect,Color c,bool filled){
    if(filled){
        Vec2 tl=renderer_world_to_pixel(r,vec2(rect.x,rect.y));
        Vec2 br=renderer_world_to_pixel(r,vec2(rect.x+rect.w,rect.y+rect.h));
        int x0=(int)tl.x,y0=(int)tl.y,x1=(int)br.x,y1=(int)br.y;
        if(x0>x1){int t=x0;x0=x1;x1=t;} if(y0>y1){int t=y0;y0=y1;y1=t;}
        for(int y=y0;y<y1;y++) for(int x=x0;x<x1;x++) pb_blend(r,x,y,c);
    }else{
        Vec2 tl=vec2(rect.x,rect.y),tr=vec2(rect.x+rect.w,rect.y);
        Vec2 bl=vec2(rect.x,rect.y+rect.h),br=vec2(rect.x+rect.w,rect.y+rect.h);
        renderer_draw_line_aa(r,tl,tr,c);renderer_draw_line_aa(r,tr,br,c);
        renderer_draw_line_aa(r,br,bl,c);renderer_draw_line_aa(r,bl,tl,c);
    }
}

void renderer_draw_rect_gradient(Renderer* r,Rect rect,Gradient g){
    Vec2 tl=renderer_world_to_pixel(r,vec2(rect.x,rect.y));
    Vec2 br=renderer_world_to_pixel(r,vec2(rect.x+rect.w,rect.y+rect.h));
    int x0=(int)tl.x,y0=(int)tl.y,x1=(int)br.x,y1=(int)br.y;
    if(x0>x1){int t=x0;x0=x1;x1=t;} if(y0>y1){int t=y0;y0=y1;y1=t;}
    int w=x1-x0,h=y1-y0; if(w<=0||h<=0)return;
    for(int y=y0;y<y1;y++) for(int x=x0;x<x1;x++){
        float tx=(float)(x-x0)/w,ty=(float)(y-y0)/h;
        Color top=color_lerp(g.tl,g.tr,tx),bot=color_lerp(g.bl,g.br,tx);
        pb_blend(r,x,y,color_lerp(top,bot,ty));
    }
}

void renderer_draw_rect_rounded(Renderer* r,Rect rct,float radius,Color c,bool filled){
    float rad=radius;
    if(rad>rct.w*.5f)rad=rct.w*.5f; if(rad>rct.h*.5f)rad=rct.h*.5f;
    renderer_draw_rect(r,(Rect){rct.x+rad,rct.y,rct.w-2*rad,rct.h},c,filled);
    renderer_draw_rect(r,(Rect){rct.x,rct.y+rad,rct.w,rct.h-2*rad},c,filled);
    renderer_draw_circle(r,vec2(rct.x+rad,      rct.y+rad),      rad,c,filled);
    renderer_draw_circle(r,vec2(rct.x+rct.w-rad,rct.y+rad),      rad,c,filled);
    renderer_draw_circle(r,vec2(rct.x+rad,      rct.y+rct.h-rad),rad,c,filled);
    renderer_draw_circle(r,vec2(rct.x+rct.w-rad,rct.y+rct.h-rad),rad,c,filled);
}

void renderer_draw_circle(Renderer* r,Vec2 center,float radius,Color c,bool filled){
    Vec2 pc=renderer_world_to_pixel(r,center); int ri=(int)(radius*r->camera.zoom);
    int cx=(int)pc.x,cy=(int)pc.y;
    if(filled){
        int ri2=ri*ri;
        for(int dy=-ri;dy<=ri;dy++) for(int dx=-ri;dx<=ri;dx++) if(dx*dx+dy*dy<=ri2) pb_blend(r,cx+dx,cy+dy,c);
    }else{
        int x2=ri,y2=0,e=0;
        while(x2>=y2){
            pb_blend(r,cx+x2,cy+y2,c);pb_blend(r,cx+y2,cy+x2,c);pb_blend(r,cx-y2,cy+x2,c);pb_blend(r,cx-x2,cy+y2,c);
            pb_blend(r,cx-x2,cy-y2,c);pb_blend(r,cx-y2,cy-x2,c);pb_blend(r,cx+y2,cy-x2,c);pb_blend(r,cx+x2,cy-y2,c);
            y2++;e+=1+2*y2;if(2*(e-x2)+1>0){x2--;e+=1-2*x2;}
        }
    }
}

void renderer_draw_circle_aa(Renderer* r,Vec2 center,float radius,Color c){
    Vec2 pc=renderer_world_to_pixel(r,center); float sr=radius*r->camera.zoom;
    int ri=(int)(sr+2),cx=(int)pc.x,cy=(int)pc.y;
    for(int dy=-ri;dy<=ri;dy++) for(int dx=-ri;dx<=ri;dx++){
        float d=sqrtf((float)(dx*dx+dy*dy))-sr;
        if(d>1.f)continue;
        Color cc=c; cc.a=(d<0)?c.a:(Uint8)(c.a*(1.f-d));
        pb_blend(r,cx+dx,cy+dy,cc);
    }
}

void renderer_draw_circle_outline(Renderer* r,Vec2 center,float radius,float thickness,Color c){
    renderer_draw_ring(r,center,radius-thickness*.5f,radius+thickness*.5f,c);
}

void renderer_draw_ellipse(Renderer* r,Vec2 center,float rx,float ry,Color c,bool filled){
    Vec2 pc=renderer_world_to_pixel(r,center);
    float srx=rx*r->camera.zoom,sry=ry*r->camera.zoom;
    int bx=(int)(srx+1),by=(int)(sry+1);
    int cx=(int)pc.x,cy=(int)pc.y;
    for(int dy=-by;dy<=by;dy++) for(int dx=-bx;dx<=bx;dx++){
        float d=(float)(dx*dx)/(srx*srx)+(float)(dy*dy)/(sry*sry);
        if(filled){if(d<=1.f)pb_blend(r,cx+dx,cy+dy,c);}
        else{if(d<=1.08f&&d>=0.92f)pb_blend(r,cx+dx,cy+dy,c);}
    }
}

void renderer_draw_ring(Renderer* r,Vec2 center,float inner,float outer,Color c){
    Vec2 pc=renderer_world_to_pixel(r,center);
    float ir=inner*r->camera.zoom,or2=outer*r->camera.zoom;
    int ri=(int)(or2+1),cx=(int)pc.x,cy=(int)pc.y;
    float ir2=ir*ir,or22=or2*or2;
    for(int dy=-ri;dy<=ri;dy++) for(int dx=-ri;dx<=ri;dx++){
        float d=(float)(dx*dx+dy*dy); if(d<ir2||d>or22)continue;
        pb_blend(r,cx+dx,cy+dy,c);
    }
}

void renderer_draw_arc(Renderer* r,Vec2 center,float radius,float a0,float a1,Color c){
    int steps=(int)(fabsf(a1-a0)*radius*r->camera.zoom+8);
    for(int i=0;i<steps;i++){
        float ta=a0+(a1-a0)*(float)i/steps, tb=a0+(a1-a0)*(float)(i+1)/steps;
        renderer_draw_line_aa(r,vec2_add(center,vec2_mul(vec2_from_angle(ta),radius)),
                                vec2_add(center,vec2_mul(vec2_from_angle(tb),radius)),c);
    }
}

void renderer_draw_sector(Renderer* r,Vec2 center,float radius,float a0,float a1,Color c){
    Vec2 pc=renderer_world_to_pixel(r,center); float sr=radius*r->camera.zoom;
    int ri=(int)(sr+1),cx=(int)pc.x,cy=(int)pc.y; float sr2=sr*sr;
    float span=fmodf(a1-a0+6.28318f,6.28318f);
    for(int dy=-ri;dy<=ri;dy++) for(int dx=-ri;dx<=ri;dx++){
        if((float)(dx*dx+dy*dy)>sr2)continue;
        float ang=atan2f((float)dy,(float)dx);
        if(fmodf(ang-a0+6.28318f,6.28318f)<=span)pb_blend(r,cx+dx,cy+dy,c);
    }
}

void renderer_draw_triangle(Renderer* r,Vec2 a,Vec2 b,Vec2 c2,Color col,bool filled){
    if(!filled){renderer_draw_line_aa(r,a,b,col);renderer_draw_line_aa(r,b,c2,col);renderer_draw_line_aa(r,c2,a,col);return;}
    Vec2 pa=renderer_world_to_pixel(r,a),pb=renderer_world_to_pixel(r,b),pc=renderer_world_to_pixel(r,c2);
    if(pa.y>pb.y){Vec2 t=pa;pa=pb;pb=t;} if(pa.y>pc.y){Vec2 t=pa;pa=pc;pc=t;} if(pb.y>pc.y){Vec2 t=pb;pb=pc;pc=t;}
    int y0=(int)pa.y,y1=(int)pb.y,y2=(int)pc.y;
    float d02=(pc.y-pa.y)>0?(pc.x-pa.x)/(pc.y-pa.y):0;
    float d01=(pb.y-pa.y)>0?(pb.x-pa.x)/(pb.y-pa.y):0;
    float d12=(pc.y-pb.y)>0?(pc.x-pb.x)/(pc.y-pb.y):0;
    float xa=pa.x,xb=pa.x;
    for(int y=y0;y<=y2;y++){
        int xl=(int)(xa<xb?xa:xb),xr=(int)(xa<xb?xb:xa);
        for(int x=xl;x<=xr;x++) pb_blend(r,x,y,col);
        xa+=d02; if(y<y1)xb+=d01; else xb+=d12;
    }
}

void renderer_draw_triangle_gradient(Renderer* r,Vec2 a,Vec2 b,Vec2 c2,Color ca,Color cb,Color cc){
    Vec2 pa=renderer_world_to_pixel(r,a),pb=renderer_world_to_pixel(r,b),pc=renderer_world_to_pixel(r,c2);
    int minx=(int)fminf(pa.x,fminf(pb.x,pc.x)),maxx=(int)fmaxf(pa.x,fmaxf(pb.x,pc.x));
    int miny=(int)fminf(pa.y,fminf(pb.y,pc.y)),maxy=(int)fmaxf(pa.y,fmaxf(pb.y,pc.y));
    float denom=(pb.y-pc.y)*(pa.x-pc.x)+(pc.x-pb.x)*(pa.y-pc.y);
    if(fabsf(denom)<0.001f)return;
    for(int y=miny;y<=maxy;y++) for(int x=minx;x<=maxx;x++){
        float w0=((pb.y-pc.y)*(x-pc.x)+(pc.x-pb.x)*(y-pc.y))/denom;
        float w1=((pc.y-pa.y)*(x-pc.x)+(pa.x-pc.x)*(y-pc.y))/denom;
        float w2=1.f-w0-w1; if(w0<0||w1<0||w2<0)continue;
        Color col={(Uint8)(ca.r*w0+cb.r*w1+cc.r*w2),(Uint8)(ca.g*w0+cb.g*w1+cc.g*w2),(Uint8)(ca.b*w0+cb.b*w1+cc.b*w2),255};
        pb_blend(r,x,y,col);
    }
}

void renderer_draw_polygon(Renderer* r,Vec2* verts,int n,Color c,bool filled){
    if(n<3)return;
    if(filled) for(int i=1;i<n-1;i++) renderer_draw_triangle(r,verts[0],verts[i],verts[i+1],c,true);
    else for(int i=0;i<n;i++) renderer_draw_line_aa(r,verts[i],verts[(i+1)%n],c);
}

void renderer_draw_capsule(Renderer* r,Vec2 a,Vec2 b,float radius,Color c){
    Vec2 dir=vec2_sub(b,a); float len=vec2_length(dir);
    if(len<0.001f){renderer_draw_circle(r,a,radius,c,true);return;}
    Vec2 n2=vec2_mul(vec2_perp(vec2_normalize(dir)),radius);
    Vec2 q[4]={vec2_add(a,n2),vec2_sub(a,n2),vec2_sub(b,n2),vec2_add(b,n2)};
    renderer_draw_triangle(r,q[0],q[1],q[2],c,true);
    renderer_draw_triangle(r,q[0],q[2],q[3],c,true);
    renderer_draw_circle(r,a,radius,c,true);
    renderer_draw_circle(r,b,radius,c,true);
}

void renderer_draw_arrow(Renderer* r,Vec2 from,Vec2 to,float head,Color c){
    renderer_draw_line_aa(r,from,to,c);
    Vec2 dir=vec2_normalize(vec2_sub(from,to));
    Vec2 perp=vec2_mul(vec2_perp(dir),head*.4f);
    renderer_draw_triangle(r,to,vec2_add(vec2_add(to,vec2_mul(dir,head)),perp),
                              vec2_sub(vec2_add(to,vec2_mul(dir,head)),perp),c,true);
}

void renderer_draw_shadow(Renderer* r,Vec2 pos,float radius,float opacity){
    Vec2 sp=vec2_add(pos,vec2(3,4));
    for(int i=0;i<3;i++){
        Color sc={0,0,0,(Uint8)(opacity*80/(1<<i))};
        renderer_draw_circle(r,sp,radius+i*2,sc,true);
    }
}

void renderer_draw_aabb(Renderer* r,AABB b,Color c){
    renderer_draw_rect(r,(Rect){b.min.x,b.min.y,b.max.x-b.min.x,b.max.y-b.min.y},c,false);
}

void renderer_draw_obb(Renderer* r,OBB o,Color c){
    Vec2 ax=vec2_mul(vec2_from_angle(o.angle),o.half_extents.x);
    Vec2 ay=vec2_mul(vec2_perp(vec2_from_angle(o.angle)),o.half_extents.y);
    Vec2 verts[4]={vec2_add(o.center,vec2_add(ax,ay)),vec2_add(o.center,vec2_sub(ax,ay)),
                   vec2_sub(o.center,vec2_add(ax,ay)),vec2_sub(o.center,vec2_sub(ax,ay))};
    renderer_draw_polygon(r,verts,4,c,false);
}

void renderer_draw_bezier(Renderer* r,Vec2 p0,Vec2 p1,Vec2 p2,int steps,Color c){
    Vec2 prev=p0;
    for(int i=1;i<=steps;i++){float t=(float)i/steps;Vec2 cur=bezier2(p0,p1,p2,t);renderer_draw_line_aa(r,prev,cur,c);prev=cur;}
}

void renderer_draw_bezier3(Renderer* r,Vec2 p0,Vec2 p1,Vec2 p2,Vec2 p3,int steps,Color c){
    Vec2 prev=p0;
    for(int i=1;i<=steps;i++){float t=(float)i/steps;Vec2 cur=bezier3(p0,p1,p2,p3,t);renderer_draw_line_aa(r,prev,cur,c);prev=cur;}
}

void renderer_draw_spline(Renderer* r,Vec2* points,int n,int sps,Color c){
    if(n<2)return;
    for(int i=0;i<n-1;i++){
        Vec2 p0=points[i>0?i-1:i],p1=points[i],p2=points[i+1],p3=points[i+2<n?i+2:i+1];
        Vec2 prev=p1;
        for(int s=1;s<=sps;s++){float t=(float)s/sps;Vec2 cur=catmull_rom(p0,p1,p2,p3,t);renderer_draw_line_aa(r,prev,cur,c);prev=cur;}
    }
}

/* ======================== ТЕКСТУРЫ ======================== */
Texture* renderer_load_texture(Renderer* r,const char* path){
    Texture* tex=(Texture*)malloc(sizeof(Texture));
    SDL_Surface* surf=SDL_LoadBMP(path);
    if(!surf){free(tex);return NULL;}
    tex->texture=SDL_CreateTextureFromSurface(r->sdl_renderer,surf);
    tex->width=surf->w; tex->height=surf->h; tex->channels=3;
    SDL_FreeSurface(surf); return tex;
}

void renderer_draw_texture(Renderer* r,Texture* tex,Vec2 pos,float rot,float scale){
    Vec2 sp=renderer_world_to_screen(r,pos);
    SDL_Rect dest={(int)(sp.x-tex->width*scale*.5f),(int)(sp.y-tex->height*scale*.5f),(int)(tex->width*scale),(int)(tex->height*scale)};
    SDL_RenderCopyEx(r->sdl_renderer,tex->texture,NULL,&dest,rot*180.f/3.14159f,NULL,SDL_FLIP_NONE);
}

void renderer_draw_texture_ex(Renderer* r,Texture* tex,Vec2 pos,float rot,Vec2 scale,Vec2 pivot,Color tint,bool flip_x,bool flip_y){
    Vec2 sp=renderer_world_to_screen(r,pos);
    SDL_Rect dest={(int)(sp.x-tex->width*scale.x*pivot.x),(int)(sp.y-tex->height*scale.y*pivot.y),(int)(tex->width*scale.x),(int)(tex->height*scale.y)};
    SDL_Point cp={(int)(tex->width*scale.x*pivot.x),(int)(tex->height*scale.y*pivot.y)};
    SDL_SetTextureColorMod(tex->texture,tint.r,tint.g,tint.b); SDL_SetTextureAlphaMod(tex->texture,tint.a);
    SDL_RenderCopyEx(r->sdl_renderer,tex->texture,NULL,&dest,rot*180.f/3.14159f,&cp,(SDL_RendererFlip)((flip_x?SDL_FLIP_HORIZONTAL:0)|(flip_y?SDL_FLIP_VERTICAL:0)));
    SDL_SetTextureColorMod(tex->texture,255,255,255); SDL_SetTextureAlphaMod(tex->texture,255);
}

void renderer_draw_texture_rect(Renderer* r,Texture* tex,TexRect src,Vec2 pos,float scale,Color tint){
    Vec2 sp=renderer_world_to_screen(r,pos);
    SDL_Rect srect={src.x,src.y,src.w,src.h};
    SDL_Rect dest={(int)sp.x,(int)sp.y,(int)(src.w*scale),(int)(src.h*scale)};
    SDL_SetTextureColorMod(tex->texture,tint.r,tint.g,tint.b); SDL_SetTextureAlphaMod(tex->texture,tint.a);
    SDL_RenderCopy(r->sdl_renderer,tex->texture,&srect,&dest);
    SDL_SetTextureColorMod(tex->texture,255,255,255); SDL_SetTextureAlphaMod(tex->texture,255);
}

void renderer_destroy_texture(Texture* tex){ if(tex){if(tex->texture)SDL_DestroyTexture(tex->texture);free(tex);} }

/* ======================== BITMAP TEXT 5x7 ======================== */
static const Uint8 _fnt[96][7]={
{0,0,0,0,0,0,0},{4,4,4,4,0,0,4},{10,10,0,0,0,0,0},{10,10,31,10,31,10,10},{4,15,20,14,5,30,4},{24,25,2,4,8,19,3},{12,18,20,8,21,18,13},{4,4,0,0,0,0,0},
{2,4,8,8,8,4,2},{8,4,2,2,2,4,8},{0,4,21,14,21,4,0},{0,4,4,31,4,4,0},{0,0,0,0,0,4,8},{0,0,0,31,0,0,0},{0,0,0,0,0,0,4},{1,1,2,4,8,16,16},
{14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{31,2,4,2,1,17,14},{2,6,10,18,31,2,2},{31,16,30,1,1,17,14},{6,8,16,30,17,17,14},{31,1,2,4,8,8,8},
{14,17,17,14,17,17,14},{14,17,17,15,1,2,12},{0,0,4,0,4,0,0},{0,0,4,0,4,4,0},{2,4,8,16,8,4,2},{0,0,31,0,31,0,0},{8,4,2,1,2,4,8},{14,17,1,2,4,0,4},
{0,0,0,0,0,0,0},{14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},{28,18,17,17,17,18,28},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
{14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},{7,2,2,2,2,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},{17,27,21,17,17,17,17},
{17,17,25,21,19,17,17},{14,17,17,17,17,17,14},{30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},
{17,17,17,17,17,17,14},{17,17,17,17,17,10,4},{17,17,17,21,21,27,17},{17,17,10,4,10,17,17},{17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
{6,4,4,4,4,4,6},{16,16,8,4,2,1,1},{6,2,2,2,2,2,6},{4,10,17,0,0,0,0},{0,0,0,0,0,0,31},{4,4,0,0,0,0,0},
{0,0,14,1,15,17,15},{16,16,30,17,17,17,30},{0,0,14,16,16,17,14},{1,1,15,17,17,17,15},{0,0,14,17,31,16,14},{6,8,30,8,8,8,8},
{0,15,17,17,15,1,14},{16,16,22,25,17,17,17},{4,0,12,4,4,4,14},{2,0,6,2,2,18,12},{16,16,18,20,24,20,18},{12,4,4,4,4,4,14},{0,0,26,21,21,17,17},
{0,0,22,25,17,17,17},{0,0,14,17,17,17,14},{0,30,17,17,30,16,16},{0,15,17,17,15,1,1},{0,0,22,25,16,16,16},{0,0,14,16,14,1,30},
{8,8,30,8,8,9,6},{0,0,17,17,17,19,13},{0,0,17,17,17,10,4},{0,0,17,21,21,21,10},{0,0,17,10,4,10,17},{0,17,17,15,1,17,14},{0,0,31,2,4,8,31},
{6,8,8,16,8,8,6},{4,4,4,0,4,4,4},{12,2,2,1,2,2,12},{8,21,2,0,0,0,0},{31,31,31,31,31,31,31}
};

void renderer_draw_text(Renderer* r,const char* text,Vec2 pos,Color c,float scale){
    Vec2 pp=renderer_world_to_pixel(r,pos); int sc=(int)fmaxf(1.f,scale);
    int cx=(int)pp.x,cy=(int)pp.y;
    for(int ci=0;text[ci];ci++){
        int ch=(int)(unsigned char)text[ci]-32; if(ch<0||ch>=96)ch=0;
        for(int row=0;row<7;row++) for(int col=0;col<5;col++)
            if(_fnt[ch][row]&(1<<(4-col)))
                for(int dy=0;dy<sc;dy++) for(int dx=0;dx<sc;dx++) pb_blend(r,cx+col*sc+dx,cy+row*sc+dy,c);
        cx+=(5+1)*sc;
    }
}

void renderer_draw_text_centered(Renderer* r,const char* text,Vec2 center,Color c,float scale){
    int len=0; for(int i=0;text[i];i++)len++;
    int sc=(int)fmaxf(1.f,scale); float w=(float)(len*(5+1)*sc);
    renderer_draw_text(r,text,vec2_add(center,vec2(-w*.5f/r->camera.zoom,-3.5f*(float)sc/r->camera.zoom)),c,scale);
}

Vec2 renderer_measure_text(const char* text,float scale){
    int len=0; for(int i=0;text[i];i++)len++; int sc=(int)fmaxf(1.f,scale);
    return vec2((float)(len*(5+1)*sc),(float)(7*sc));
}

void renderer_draw_int(Renderer* r,int val,Vec2 pos,Color c,float scale){
    char buf[32]; snprintf(buf,32,"%d",val); renderer_draw_text(r,buf,pos,c,scale);
}

void renderer_draw_float(Renderer* r,float val,int dec,Vec2 pos,Color c,float scale){
    char fmt[16],buf[32]; snprintf(fmt,16,"%%.%df",dec); snprintf(buf,32,fmt,val);
    renderer_draw_text(r,buf,pos,c,scale);
}

/* ======================== ФИЗИКА → РЕНДЕР ======================== */
void renderer_draw_particle(Renderer* r,Particle* p){
    if(!p||!p->active)return;
    Color c; switch(p->type){
        case PARTICLE_TYPE_WATER:     c=COLOR_BLUE;    break;
        case PARTICLE_TYPE_SAND:      c=COLOR_YELLOW;  break;
        case PARTICLE_TYPE_GAS:       c=COLOR_CYAN;    break;
        case PARTICLE_TYPE_CLOTH:     c=COLOR_GREEN;   break;
        case PARTICLE_TYPE_SOFT_BODY: c=COLOR_MAGENTA; break;
        case PARTICLE_TYPE_STATIC:    c=COLOR_GRAY;    break;
        default: c=COLOR_WHITE; break;
    }
    renderer_draw_circle(r,p->position,p->radius,c,true);
}

void renderer_draw_spring(Renderer* r,Spring* s,Color c){
    if(!s||!s->active)return;
    float dist=vec2_distance(s->a->position,s->b->position);
    float stretch=dist/(s->rest_distance>0.001f?s->rest_distance:1.f);
    if(stretch>1.2f)c=color_rgb(255,100,0);
    else if(stretch>1.f)c=color_lerp(c,color_rgb(255,200,0),(stretch-1.f)*5.f);
    renderer_draw_line(r,s->a->position,s->b->position,c);
}

void renderer_draw_cloth(Renderer* r,Cloth* cloth){
    if(!cloth)return;
    Color lc=color_rgb(80,200,80);
    for(int y=0;y<cloth->height;y++) for(int x=0;x<cloth->width;x++){
        Particle* p=cloth->particles[y*cloth->width+x];
        if(x<cloth->width-1)  renderer_draw_line(r,p->position,cloth->particles[y*cloth->width+x+1]->position,lc);
        if(y<cloth->height-1) renderer_draw_line(r,p->position,cloth->particles[(y+1)*cloth->width+x]->position,lc);
    }
}

void renderer_draw_softbody(Renderer* r,SoftBody* body){
    if(!body)return;
    for(int i=0;i<body->particle_count;i++) renderer_draw_particle(r,body->particles[i]);
}

void renderer_draw_trigger_zone(Renderer* r,TriggerZone* t,Color c){
    if(!t->active)return;
    Rect b={t->bounds.min.x,t->bounds.min.y,t->bounds.max.x-t->bounds.min.x,t->bounds.max.y-t->bounds.min.y};
    Color f=c;f.a=40;renderer_draw_rect(r,b,f,true);c.a=200;renderer_draw_rect(r,b,c,false);
}

void renderer_draw_force_field(Renderer* r,ForceField* ff){
    if(!ff->active)return;
    Color cols[]={{255,100,100,120},{100,100,255,120},{100,255,100,120},{255,200,50,120},{200,200,200,80},{200,100,255,100},{100,200,255,80}};
    Color c=cols[ff->type<7?(int)ff->type:0];
    if(ff->radius>0)renderer_draw_circle_aa(r,ff->position,ff->radius,c);
}

void renderer_debug_particle(Renderer* r,Particle* p,bool show_vel,bool show_forces){
    if(!p||!p->active)return;
    renderer_draw_circle(r,p->position,p->radius,color_rgba(255,0,0,80),false);
    if(show_vel)renderer_draw_arrow(r,p->position,vec2_add(p->position,vec2_mul(p->velocity,.1f)),3,color_rgb(0,255,0));
    if(show_forces)renderer_draw_arrow(r,p->position,vec2_add(p->position,vec2_mul(p->force,.01f)),3,color_rgb(255,150,0));
}

void renderer_debug_physics(Renderer* r,PhysicsWorld* w){
    for(int i=0;i<w->particle_count;i++) renderer_debug_particle(r,w->particles[i],false,false);
    for(int i=0;i<w->spring_count;i++) renderer_draw_spring(r,w->springs[i],color_rgb(100,200,100));
    for(int i=0;i<w->trigger_count;i++) renderer_draw_trigger_zone(r,&w->triggers[i],color_rgba(0,200,255,100));
    for(int i=0;i<w->force_field_count;i++) renderer_draw_force_field(r,&w->force_fields[i]);
}

void renderer_debug_raycast(Renderer* r,Vec2 origin,Vec2 dir,float max_dist,RaycastHit* hit){
    Vec2 end=(hit&&hit->hit)?hit->point:vec2_add(origin,vec2_mul(vec2_normalize(dir),max_dist));
    renderer_draw_line_aa(r,origin,end,color_rgba(255,255,0,180));
    if(hit&&hit->hit){renderer_draw_circle_aa(r,hit->point,4,color_rgb(255,80,0));renderer_draw_arrow(r,hit->point,vec2_add(hit->point,vec2_mul(hit->normal,15)),3,color_rgb(0,200,255));}
}

/* ======================== ОГОНЬ ======================== */
FireSim* fire_create(int hotspots){
    FireSim* f=(FireSim*)calloc(1,sizeof(FireSim));
    f->intensity=1.f; if(hotspots>MAX_FIRE_HOTSPOTS)hotspots=MAX_FIRE_HOTSPOTS;
    f->hotspot_count=hotspots;
    for(int i=0;i<hotspots;i++){f->hotspot_x[i]=PB_W*.1f+random_float()*(PB_W*.8f);f->hotspot_phase[i]=random_float()*6.28318f;}
    static const Uint8 pr[]={0,10,30,60,100,150,200,220,230,240,255,255,255,255,255,255};
    static const Uint8 pg[]={0, 0, 0, 0,  0, 10, 30, 60,100,150,200,220,235,245,250,255};
    static const Uint8 pb[]={0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0, 30, 80,140,200,230};
    for(int i=0;i<256;i++){int s=i*15/255;f->palette[i]=color_rgb(pr[s],pg[s],pb[s]);}
    return f;
}
void fire_destroy(FireSim* f){free(f);}
void fire_set_wind(FireSim* f,float w){f->wind=w;}
void fire_set_intensity(FireSim* f,float i){f->intensity=i;}
void fire_add_hotspot(FireSim* f,float nx){if(f->hotspot_count<MAX_FIRE_HOTSPOTS){int i=f->hotspot_count++;f->hotspot_x[i]=nx;f->hotspot_phase[i]=random_float()*6.28318f;}}

void fire_update(FireSim* f,float dt){
    f->t+=dt;
    int base=(PB_H-1)*PB_W;
    for(int x=0;x<PB_W;x++){
        float n=noise1d((float)x*.05f,f->t)*30.f; float heat=0;
        for(int i=0;i<f->hotspot_count;i++){
            f->hotspot_phase[i]+=.005f+random_float()*.002f;
            float dx=(float)x-f->hotspot_x[i];
            heat+=(0.75f+0.25f*sinf(f->hotspot_phase[i]))*140.f*f->intensity/(1.f+dx*dx*.004f);
        }
        if(random_float()<.02f)heat+=30.f+random_float()*60.f;
        heat+=n; if(heat>255)heat=255; if(heat<0)heat=0; f->buf[base+x]=heat;
    }
    for(int y=0;y<PB_H-1;y++){
        float decay=.3f+(1.f-(float)y/PB_H)*1.2f;
        for(int x=0;x<PB_W;x++){
            float c=f->buf[(y+1)*PB_W+x];
            float cl=(x>0)?f->buf[(y+1)*PB_W+x-1]:c;
            float cr=(x<PB_W-1)?f->buf[(y+1)*PB_W+x+1]:c;
            float cd=(y+2<PB_H)?f->buf[(y+2)*PB_W+x]:c;
            float s=c*2+(cl+cr)*.15f+cd*.7f;
            int xw=x+(int)(f->wind*.5f); if(xw>=0&&xw<PB_W)s+=f->buf[(y+1)*PB_W+xw]*.1f;
            s/=3.1f; float v=s-decay+(random_float()-.5f)*1.f;
            if(v>255)v=255; if(v<0)v=0; f->buf[y*PB_W+x]=v;
        }
    }
}

void fire_render(Renderer* r,FireSim* f,int px,int py,int pw,int ph){
    for(int sy=0;sy<PB_H;sy++){
        int dy=py+sy*ph/PB_H; if(dy<0||dy>=PB_H)continue;
        for(int sx=0;sx<PB_W;sx++){
            int dx=px+sx*pw/PB_W; if(dx<0||dx>=PB_W)continue;
            float fv=f->buf[sy*PB_W+sx]; if(fv<4)continue;
            int idx=(int)(fv/16)*16; if(idx>255)idx=255;
            Color col=f->palette[idx]; col.a=(Uint8)((fv/255.f)*(fv/255.f)*255);
            pb_blend(r,dx,dy,col);
        }
    }
}

/* ======================== ДЫМ ======================== */
SmokeSim* smoke_create(void){SmokeSim* s=(SmokeSim*)calloc(1,sizeof(SmokeSim));s->color=color_rgba(60,55,52,30);return s;}
void smoke_destroy(SmokeSim* s){free(s);}
void smoke_emit(SmokeSim* s,float x,float y,float wind){
    for(int i=0;i<MAX_SMOKE_PARTS;i++) if(s->parts[i].life<=0){
        s->parts[i].x=x+(random_float()-.5f)*10; s->parts[i].y=y;
        s->parts[i].vx=wind*.3f+(random_float()-.5f)*.3f; s->parts[i].vy=-(0.04f+random_float()*.05f);
        s->parts[i].life=1.f; s->parts[i].decay=.003f+random_float()*.004f; s->parts[i].size=3+random_float()*6; return;
    }
}
void smoke_emit_color(SmokeSim* s,float x,float y,float wind,Color c){smoke_emit(s,x,y,wind);(void)c;}
void smoke_update(SmokeSim* s,float dt,float wind){
    for(int i=0;i<MAX_SMOKE_PARTS;i++){if(s->parts[i].life<=0)continue;s->parts[i].life-=s->parts[i].decay;s->parts[i].x+=s->parts[i].vx+wind*.15f;s->parts[i].y+=s->parts[i].vy;s->parts[i].vx+=(random_float()-.5f)*.04f;s->parts[i].size+=.006f;if(s->parts[i].life<0)s->parts[i].life=0;}
    (void)dt;
}
void smoke_render(Renderer* r,SmokeSim* s){
    for(int i=0;i<MAX_SMOKE_PARTS;i++){if(s->parts[i].life<=0)continue;int rad=(int)s->parts[i].size,cx=(int)s->parts[i].x,cy=(int)s->parts[i].y;Color sc=s->color;sc.a=(Uint8)(s->parts[i].life*30);for(int dy=-rad;dy<=rad;dy++)for(int dx=-rad;dx<=rad;dx++){if(dx*dx+dy*dy>rad*rad)continue;pb_blend(r,cx+dx,cy+dy,sc);}}
}

/* ======================== ДОЖДЬ ======================== */
RainSim* rain_create(void){
    RainSim* rn=(RainSim*)calloc(1,sizeof(RainSim));
    for(int i=0;i<MAX_RAIN_DROPS;i++){rn->drops[i].x=random_float()*PB_W;rn->drops[i].y=random_float()*PB_H;rn->drops[i].vy=1.5f+random_float();rn->drops[i].len=3+random_float()*4;}
    rn->active=true;rn->intensity=1;rn->color=color_rgba(150,180,220,100);return rn;
}
void rain_destroy(RainSim* rn){free(rn);}
void rain_set_intensity(RainSim* rn,float i){rn->intensity=clamp(i,0,1);}
void rain_update(RainSim* rn,float dt,float wind){
    if(!rn->active)return;
    for(int i=0;i<(int)(MAX_RAIN_DROPS*rn->intensity);i++){rn->drops[i].y+=rn->drops[i].vy;rn->drops[i].x+=wind*.25f+rn->drops[i].vx;if(rn->drops[i].y>PB_H||rn->drops[i].x<0||rn->drops[i].x>PB_W){rn->drops[i].x=random_float()*PB_W;rn->drops[i].y=-rn->drops[i].len;}}
    (void)dt;
}
void rain_render(Renderer* r,RainSim* rn,float wind){
    if(!rn->active)return;
    for(int i=0;i<(int)(MAX_RAIN_DROPS*rn->intensity);i++){int x=(int)rn->drops[i].x,y=(int)rn->drops[i].y;int ex=(int)(x+wind*.4f),ey=(int)(y-rn->drops[i].len);int dx2=ex-x,dy2=ey-y,st=abs(dy2)>abs(dx2)?abs(dy2):abs(dx2);if(!st)continue;for(int s=0;s<=st;s++)pb_blend(r,x+dx2*s/st,y+dy2*s/st,rn->color);}
}

/* ======================== ИСКРЫ ======================== */
SparkSim* spark_create(void){return (SparkSim*)calloc(1,sizeof(SparkSim));}
void spark_destroy(SparkSim* s){free(s);}
void spark_emit(SparkSim* s,float x,float y,float wind){
    for(int i=0;i<MAX_SPARK_PARTS;i++) if(s->parts[i].life<=0){
        s->parts[i].x=x+(random_float()-.5f)*15;s->parts[i].y=y;
        s->parts[i].vx=wind*.35f+(random_float()-.5f)*1.f;s->parts[i].vy=-(0.3f+random_float()*.4f);
        s->parts[i].life=0.8f+random_float()*.2f;s->parts[i].decay=.01f+random_float()*.01f;s->parts[i].color=COLOR_WHITE;
        for(int j=0;j<SPARK_TRAIL;j++){s->parts[i].tx[j]=x;s->parts[i].ty[j]=y;}return;
    }
}
void spark_emit_color(SparkSim* s,float x,float y,float wind,Color c,float spd){
    for(int i=0;i<MAX_SPARK_PARTS;i++) if(s->parts[i].life<=0){
        s->parts[i].x=x+(random_float()-.5f)*15;s->parts[i].y=y;
        s->parts[i].vx=wind*.35f+(random_float()-.5f)*spd;s->parts[i].vy=-(0.3f+random_float()*.4f)*spd;
        s->parts[i].life=0.8f+random_float()*.2f;s->parts[i].decay=.01f+random_float()*.01f;s->parts[i].color=c;
        for(int j=0;j<SPARK_TRAIL;j++){s->parts[i].tx[j]=x;s->parts[i].ty[j]=y;}return;
    }
}
void spark_update(SparkSim* s,float dt,float wind){
    for(int i=0;i<MAX_SPARK_PARTS;i++){if(s->parts[i].life<=0)continue;for(int j=SPARK_TRAIL-1;j>0;j--){s->parts[i].tx[j]=s->parts[i].tx[j-1];s->parts[i].ty[j]=s->parts[i].ty[j-1];}s->parts[i].tx[0]=s->parts[i].x;s->parts[i].ty[0]=s->parts[i].y;s->parts[i].life-=s->parts[i].decay;s->parts[i].x+=s->parts[i].vx+wind*.2f;s->parts[i].y+=s->parts[i].vy;s->parts[i].vy+=.007f;s->parts[i].vx+=(random_float()-.5f)*.08f;if(s->parts[i].life<0)s->parts[i].life=0;}
    (void)dt;
}
void spark_render(Renderer* r,SparkSim* s){
    for(int i=0;i<MAX_SPARK_PARTS;i++){if(s->parts[i].life<=0)continue;float l=s->parts[i].life;Color bc=s->parts[i].color;for(int j=0;j<SPARK_TRAIL;j++){Color tc=bc;tc.a=(Uint8)(l*(1.f-(float)j/SPARK_TRAIL)*120);pb_blend(r,(int)s->parts[i].tx[j],(int)s->parts[i].ty[j],tc);}Color sc=bc;sc.a=(Uint8)(l*255);int sx=(int)s->parts[i].x,sy=(int)s->parts[i].y;pb_blend(r,sx,sy,sc);pb_blend(r,sx+1,sy,sc);pb_blend(r,sx-1,sy,sc);pb_blend(r,sx,sy+1,sc);pb_blend(r,sx,sy-1,sc);}
}

/* ======================== СНЕГ ======================== */
SnowSim* snow_create(int count){
    SnowSim* s=(SnowSim*)calloc(1,sizeof(SnowSim));
    s->count=count<MAX_SNOW?count:MAX_SNOW;s->gravity=0.03f;s->active=true;s->color=COLOR_WHITE;
    for(int i=0;i<s->count;i++){s->flakes[i].x=random_float()*PB_W;s->flakes[i].y=random_float()*PB_H;s->flakes[i].vx=(random_float()-.5f)*.5f;s->flakes[i].vy=0.1f+random_float()*.3f;s->flakes[i].size=0.5f+random_float()*1.5f;}
    return s;
}
void snow_destroy(SnowSim* s){free(s);}
void snow_set_wind(SnowSim* s,float wind){s->wind=wind;}
void snow_update(SnowSim* s,float dt){
    if(!s->active)return;
    for(int i=0;i<s->count;i++){
        s->flakes[i].x+=s->flakes[i].vx+s->wind*dt;s->flakes[i].y+=s->flakes[i].vy+s->gravity;s->flakes[i].vx+=(random_float()-.5f)*.02f;
        if(s->flakes[i].y>PB_H){s->flakes[i].y=-2;s->flakes[i].x=random_float()*PB_W;}
        if(s->flakes[i].x<0)s->flakes[i].x+=PB_W;
        else if(s->flakes[i].x>PB_W)s->flakes[i].x-=PB_W;
    }
}
void snow_render(Renderer* r,SnowSim* s){
    if(!s->active)return;
    for(int i=0;i<s->count;i++){
        int x=(int)s->flakes[i].x,y=(int)s->flakes[i].y;
        pb_blend(r,x,y,s->color);
        Color c2=color_with_alpha(s->color,180);
        pb_blend(r,x-1,y,c2);pb_blend(r,x+1,y,c2);pb_blend(r,x,y-1,c2);pb_blend(r,x,y+1,c2);
    }
}

/* ======================== МОЛНИЯ ======================== */
LightningSim* lightning_create(void){return (LightningSim*)calloc(1,sizeof(LightningSim));}
void lightning_destroy(LightningSim* l){free(l);}

static void _bolt(Vec2 from,Vec2 to,Vec2* pts,int* n,int max,int depth,float disp){
    if(depth<=0||*n+2>=max){pts[(*n)++]=to;return;}
    Vec2 mid=vec2_lerp(from,to,.5f);
    mid=vec2_add(mid,vec2_mul(vec2_perp(vec2_normalize(vec2_sub(to,from))),(random_float()-.5f)*disp));
    _bolt(from,mid,pts,n,max,depth-1,disp*.5f); pts[(*n)++]=mid;
    _bolt(mid,to, pts,n,max,depth-1,disp*.5f);
}

void lightning_strike(LightningSim* l,Vec2 from,Vec2 to,int branches){
    if(l->count>=MAX_LIGHTNING)return;
    LightningSeg* seg=&l->segs[l->count++];
    seg->points[0]=from;seg->count=1;
    _bolt(from,to,seg->points,&seg->count,LIGHTNING_MAX_PTS,branches,vec2_distance(from,to)*.3f);
    seg->life=0.15f+random_float()*.1f;seg->max_life=seg->life;
    seg->color=color_rgb(200,220,255);seg->glow_color=color_rgba(100,150,255,80);
}
void lightning_update(LightningSim* l,float dt){
    for(int i=l->count-1;i>=0;i--){l->segs[i].life-=dt;if(l->segs[i].life<=0)l->segs[i]=l->segs[--l->count];}
}
void lightning_render(Renderer* r,LightningSim* l){
    for(int i=0;i<l->count;i++){LightningSeg* seg=&l->segs[i];float a=seg->life/seg->max_life;Color gc=seg->glow_color;gc.a=(Uint8)(gc.a*a);Color cc=seg->color;cc.a=(Uint8)(255*a);for(int j=0;j<seg->count-1;j++){renderer_draw_line_aa(r,seg->points[j],seg->points[j+1],gc);renderer_draw_line_aa(r,seg->points[j],seg->points[j+1],cc);}}
}

/* ======================== RIPPLE ======================== */
RippleSim* ripple_create(void){return (RippleSim*)calloc(1,sizeof(RippleSim));}
void ripple_destroy(RippleSim* rs){free(rs);}
void ripple_add(RippleSim* rs,Vec2 pos,float speed,float max_radius,Color c){
    for(int i=0;i<MAX_RIPPLES;i++) if(rs->ripples[i].life<=0){rs->ripples[i].x=pos.x;rs->ripples[i].y=pos.y;rs->ripples[i].radius=0;rs->ripples[i].max_radius=max_radius;rs->ripples[i].speed=speed;rs->ripples[i].life=1;rs->ripples[i].color=c;return;}
}
void ripple_update(RippleSim* rs,float dt){for(int i=0;i<MAX_RIPPLES;i++){if(rs->ripples[i].life<=0)continue;rs->ripples[i].radius+=rs->ripples[i].speed*dt;rs->ripples[i].life=1.f-rs->ripples[i].radius/rs->ripples[i].max_radius;}}
void ripple_render(Renderer* r,RippleSim* rs){for(int i=0;i<MAX_RIPPLES;i++){if(rs->ripples[i].life<=0)continue;Color c=rs->ripples[i].color;c.a=(Uint8)(c.a*rs->ripples[i].life);renderer_draw_circle_aa(r,vec2(rs->ripples[i].x,rs->ripples[i].y),rs->ripples[i].radius,c);}}

/* ======================== METABALL ======================== */
MetaballSim* metaball_create(void){MetaballSim* ms=(MetaballSim*)calloc(1,sizeof(MetaballSim));ms->threshold=1.f;ms->fill_color=color_rgb(0,200,0);ms->edge_color=COLOR_WHITE;return ms;}
void metaball_destroy(MetaballSim* ms){free(ms);}
void metaball_add(MetaballSim* ms,Vec2 pos,float radius,Color c){if(ms->count<MAX_METABALLS){ms->balls[ms->count].x=pos.x;ms->balls[ms->count].y=pos.y;ms->balls[ms->count].radius=radius;ms->balls[ms->count].color=c;ms->count++;}}
void metaball_set_position(MetaballSim* ms,int i,Vec2 pos){if(i<ms->count){ms->balls[i].x=pos.x;ms->balls[i].y=pos.y;}}
void metaball_update(MetaballSim* ms,float dt){(void)ms;(void)dt;}
void metaball_render(Renderer* r,MetaballSim* ms){
    for(int y=0;y<PB_H;y++) for(int x=0;x<PB_W;x++){
        float fx=(float)(x-PB_W/2)/r->camera.zoom+r->camera.position.x;
        float fy=(float)(y-PB_H/2)/r->camera.zoom+r->camera.position.y;
        float field=0; Color blended={0,0,0,0};
        for(int i=0;i<ms->count;i++){float dx=fx-ms->balls[i].x,dy=fy-ms->balls[i].y;float d2=dx*dx+dy*dy;if(d2<0.001f)d2=0.001f;float contrib=ms->balls[i].radius*ms->balls[i].radius/d2;field+=contrib;float w=fminf(1.f,contrib/ms->threshold);blended.r=(Uint8)fminf(255,blended.r+ms->balls[i].color.r*w);blended.g=(Uint8)fminf(255,blended.g+ms->balls[i].color.g*w);blended.b=(Uint8)fminf(255,blended.b+ms->balls[i].color.b*w);}
        if(field>=ms->threshold){blended.a=255;if(ms->show_edge&&field<ms->threshold*1.15f)blended=ms->edge_color;pb_blend(r,x,y,blended);}
    }
}

/* ======================== UNIVERSAL PARTICLE SYSTEM ======================== */
ParticleSystem* ps_create(void){
    ParticleSystem* ps=(ParticleSystem*)calloc(1,sizeof(ParticleSystem));
    ps->max_particles=PS_MAX_PARTICLES;ps->emit_rate=30;ps->speed_min=20;ps->speed_max=60;
    ps->life_min=1;ps->life_max=2;ps->size_min=2;ps->size_max=5;
    ps->color_start_min=color_rgb(255,255,255);ps->color_start_max=color_rgb(255,255,255);
    ps->color_end_min=color_rgba(255,100,0,0);ps->color_end_max=color_rgba(255,50,0,0);
    ps->gravity_scale=1;ps->drag=0.98f;ps->looping=true;ps->world_space=true;
    ps->emitter_spread=6.28318f;ps->emitter_direction=vec2(0,-1);
    return ps;
}
void ps_destroy(ParticleSystem* ps){free(ps);}
void ps_set_position(ParticleSystem* ps,float x,float y){ps->ex=x;ps->ey=y;}
void ps_stop(ParticleSystem* ps){ps->paused=true;}
void ps_play(ParticleSystem* ps){ps->paused=false;}
bool ps_is_alive(ParticleSystem* ps){if(!ps->paused)return true;for(int i=0;i<ps->max_particles;i++)if(ps->particles[i].active)return true;return false;}
static void _ps_spawn(ParticleSystem* ps){
    for(int i=0;i<ps->max_particles;i++){PSParticle* p=&ps->particles[i];if(p->active)continue;float life=ps->life_min+(ps->life_max-ps->life_min)*random_float();p->life=1;p->inv_life=1.f/life;p->active=true;float ex=ps->ex,ey=ps->ey;if(ps->emitter_shape==PS_EMITTER_CIRCLE){float a=random_float()*6.28318f,rd=sqrtf(random_float())*ps->emitter_radius;ex+=rd*cosf(a);ey+=rd*sinf(a);}else if(ps->emitter_shape==PS_EMITTER_RECT){ex+=(random_float()-.5f)*ps->emitter_w;ey+=(random_float()-.5f)*ps->emitter_h;}p->x=ex;p->y=ey;float speed=ps->speed_min+(ps->speed_max-ps->speed_min)*random_float();float angle=atan2f(ps->emitter_direction.y,ps->emitter_direction.x)+(random_float()-.5f)*ps->emitter_spread;p->vx=cosf(angle)*speed;p->vy=sinf(angle)*speed;p->size=ps->size_min+(ps->size_max-ps->size_min)*random_float();float cr=random_float();p->color=color_lerp(ps->color_start_min,ps->color_start_max,cr);p->color_end=color_lerp(ps->color_end_min,ps->color_end_max,cr);return;}
}
void ps_emit(ParticleSystem* ps,int count){for(int i=0;i<count;i++)_ps_spawn(ps);}
void ps_burst(ParticleSystem* ps,int count){ps_emit(ps,count);}
void ps_update(ParticleSystem* ps,float dt){
    if(!ps->paused){ps->elapsed+=dt;if(!ps->looping&&ps->duration>0&&ps->elapsed>ps->duration)ps->paused=true;ps->emit_accumulator+=ps->emit_rate*dt;while(ps->emit_accumulator>=1){_ps_spawn(ps);ps->emit_accumulator-=1;}}
    for(int i=0;i<ps->max_particles;i++){PSParticle* p=&ps->particles[i];if(!p->active)continue;p->life-=p->inv_life*dt;if(p->life<=0){p->active=false;continue;}p->vy+=9.8f*ps->gravity_scale*dt;p->vx*=ps->drag;p->vy*=ps->drag;p->x+=p->vx*dt;p->y+=p->vy*dt;}
}
void ps_render(Renderer* r,ParticleSystem* ps){
    for(int i=0;i<ps->max_particles;i++){PSParticle* p=&ps->particles[i];if(!p->active)continue;float t=1-p->life;Color c=color_lerp(p->color,p->color_end,t);float sz=p->size*p->life;if(ps->render_mode==PS_RENDER_SQUARE)pb_fill_rect(r,(int)(p->x-sz*.5f),(int)(p->y-sz*.5f),(int)sz,(int)sz,c);else renderer_draw_circle(r,vec2(p->x,p->y),(int)(sz>0?sz:1),c,true);}
}

/* ======================== TRAIL ======================== */
Trail* trail_create(float lifetime,float min_dist){Trail* t=(Trail*)calloc(1,sizeof(Trail));t->point_lifetime=lifetime;t->min_distance=min_dist;t->width_start=4;t->color_start=COLOR_WHITE;t->color_end=COLOR_TRANSPARENT;t->fade=true;return t;}
void trail_destroy(Trail* t){free(t);}
void trail_clear(Trail* t){t->count=0;t->head=0;}
void trail_add_point(Trail* t,Vec2 pos,Color c){
    if(t->count>0){int last=(t->head-1+TRAIL_MAX_POINTS)%TRAIL_MAX_POINTS;if(vec2_distance(pos,t->points[last])<t->min_distance)return;}
    t->points[t->head]=pos;t->ages[t->head]=0;t->colors[t->head]=c;t->widths[t->head]=t->width_start;
    t->head=(t->head+1)%TRAIL_MAX_POINTS;if(t->count<TRAIL_MAX_POINTS)t->count++;
}
void trail_update(Trail* t,float dt){for(int i=0;i<t->count;i++){int idx=(t->head-1-i+TRAIL_MAX_POINTS*2)%TRAIL_MAX_POINTS;t->ages[idx]+=dt;float a=1.f-t->ages[idx]/t->point_lifetime;if(a<0){t->count=i;break;}t->widths[idx]=t->width_end+(t->width_start-t->width_end)*a;}}
void trail_render(Renderer* r,Trail* t){for(int i=0;i<t->count-1;i++){int i0=(t->head-1-i+TRAIL_MAX_POINTS*2)%TRAIL_MAX_POINTS,i1=(t->head-2-i+TRAIL_MAX_POINTS*2)%TRAIL_MAX_POINTS;float alpha=1.f-t->ages[i0]/t->point_lifetime;Color c=t->fade?color_with_alpha(t->colors[i0],(Uint8)(alpha*255)):t->colors[i0];renderer_draw_capsule(r,t->points[i0],t->points[i1],t->widths[i0]*.5f,c);}}

/* ======================== TILEMAP ======================== */
Tilemap* tilemap_create(int tw,int th){Tilemap* tm=(Tilemap*)calloc(1,sizeof(Tilemap));tm->tile_w=tw;tm->tile_h=th;return tm;}
void tilemap_destroy(Tilemap* tm){for(int i=0;i<tm->layer_count;i++)if(tm->layers[i].data)free(tm->layers[i].data);free(tm);}
TilemapLayer* tilemap_add_layer(Tilemap* tm,Texture* ts,int ts_cols,int mw,int mh){if(tm->layer_count>=TILEMAP_MAX_LAYERS)return NULL;TilemapLayer* l=&tm->layers[tm->layer_count++];l->tileset=ts;l->tileset_cols=ts_cols;l->width=mw;l->height=mh;l->data=(int*)calloc(mw*mh,sizeof(int));l->visible=true;l->parallax_x=1;l->parallax_y=1;l->tint=COLOR_WHITE;return l;}
void tilemap_set_tile(TilemapLayer* l,int x,int y,int id){if(x>=0&&x<l->width&&y>=0&&y<l->height)l->data[y*l->width+x]=id;}
int  tilemap_get_tile(TilemapLayer* l,int x,int y){if(x>=0&&x<l->width&&y>=0&&y<l->height)return l->data[y*l->width+x];return 0;}
void tilemap_render_layer(Renderer* r,Tilemap* tm,int li){TilemapLayer* l=&tm->layers[li];if(!l->visible||!l->tileset)return;int tw=tm->tile_w,th=tm->tile_h,tc=l->tileset_cols;for(int y=0;y<l->height;y++)for(int x=0;x<l->width;x++){int id=l->data[y*l->width+x];if(id<=0)continue;int tid=id-1;TexRect src={(tid%tc)*tw,(tid/tc)*th,tw,th};renderer_draw_texture_rect(r,l->tileset,src,vec2((float)(x*tw),(float)(y*th)),1,l->tint);}}
void tilemap_render(Renderer* r,Tilemap* tm){for(int i=0;i<tm->layer_count;i++)tilemap_render_layer(r,tm,i);}

/* ======================== ANIMATED SPRITE ======================== */
AnimatedSprite* anim_sprite_create(Texture* sheet){AnimatedSprite* s=(AnimatedSprite*)calloc(1,sizeof(AnimatedSprite));s->sheet=sheet;s->scale=1;s->tint=COLOR_WHITE;s->pivot=vec2(.5f,.5f);return s;}
void anim_sprite_destroy(AnimatedSprite* s){free(s);}
int anim_sprite_add_clip(AnimatedSprite* s,const char* name,int sx,int sy,int fw,int fh,int fc,float fps,bool loop){if(s->clip_count>=ANIM_MAX_CLIPS)return -1;AnimClip* clip=&s->clips[s->clip_count];strncpy(clip->name,name,31);clip->fps=fps;clip->loop=loop;clip->frame_count=fc;for(int i=0;i<fc&&i<ANIM_MAX_FRAMES;i++)clip->frames[i]=(TexRect){sx+i*fw,sy,fw,fh};return s->clip_count++;}
void anim_sprite_play(AnimatedSprite* s,int ci){s->current_clip=ci;s->current_frame=0;s->frame_timer=0;s->playing=true;s->finished=false;}
void anim_sprite_play_name(AnimatedSprite* s,const char* name){for(int i=0;i<s->clip_count;i++)if(strcmp(s->clips[i].name,name)==0){anim_sprite_play(s,i);return;}}
void anim_sprite_stop(AnimatedSprite* s){s->playing=false;}
void anim_sprite_update(AnimatedSprite* s,float dt){if(!s->playing||s->finished)return;AnimClip* clip=&s->clips[s->current_clip];s->frame_timer+=dt;if(s->frame_timer>=1.f/clip->fps){s->frame_timer=0;s->current_frame++;if(s->on_frame)s->on_frame(s->current_frame,s->user_data);if(s->current_frame>=clip->frame_count){if(clip->loop)s->current_frame=0;else{s->current_frame=clip->frame_count-1;s->playing=false;s->finished=true;if(s->on_end)s->on_end(s->user_data);}}}}
void anim_sprite_render(Renderer* r,AnimatedSprite* s,Vec2 pos){if(!s->sheet)return;AnimClip* clip=&s->clips[s->current_clip];TexRect src=clip->frames[s->current_frame];renderer_draw_texture_rect(r,s->sheet,src,vec2_sub(pos,vec2(src.w*s->scale*.5f,src.h*s->scale*.5f)),s->scale,s->tint);}
