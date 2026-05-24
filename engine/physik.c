/* physik.c — Cngine v2
 * LGPL v3 | Chikatilo / sooborn / SGLauncher */
#include "physik.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ================================================================
   SPATIAL HASH — O(1) insert, O(1) lookup
================================================================ */
#define SH_CELL_SIZE 40.0f
#define SH_BUCKETS   4096
#define SH_MAX_CELL  48

typedef struct { int idx[SH_MAX_CELL]; int count; } SHCell;
typedef struct { SHCell cells[SH_BUCKETS]; } SpatialHash;

static SpatialHash* sh_create(void){ return (SpatialHash*)calloc(1,sizeof(SpatialHash)); }
static void sh_clear(SpatialHash* sh){ memset(sh->cells,0,sizeof(sh->cells)); }
static int sh_hash(int cx,int cy){ unsigned h=(unsigned)(cx*73856093)^(unsigned)(cy*19349663); return (int)(h&(SH_BUCKETS-1)); }
static void sh_insert(SpatialHash* sh,int i,float x,float y){
    int cx=(int)floorf(x/SH_CELL_SIZE),cy=(int)floorf(y/SH_CELL_SIZE);
    SHCell* c=&sh->cells[sh_hash(cx,cy)];
    if(c->count<SH_MAX_CELL)c->idx[c->count++]=i;
}

/* ================================================================
   ПУЛА ЧАСТИЦ — без malloc/free в рантайме
================================================================ */
#define POOL_SIZE 8192

typedef struct {
    Particle slots[POOL_SIZE];
    bool     used[POOL_SIZE];
    int      free_list[POOL_SIZE];
    int      free_count;
} ParticlePool;

static ParticlePool* pool_create(void){
    ParticlePool* p=(ParticlePool*)calloc(1,sizeof(ParticlePool));
    p->free_count=POOL_SIZE;
    for(int i=0;i<POOL_SIZE;i++) p->free_list[i]=POOL_SIZE-1-i;
    return p;
}
static Particle* pool_alloc(ParticlePool* p){
    if(p->free_count==0)return NULL;
    int i=p->free_list[--p->free_count];
    p->used[i]=true;
    return &p->slots[i];
}
static void pool_free(ParticlePool* p,Particle* part){
    int i=(int)(part-p->slots);
    if(i<0||i>=POOL_SIZE)return;
    p->used[i]=false;
    p->free_list[p->free_count++]=i;
}

/* ================================================================
   ГЛОБАЛЬНЫЕ (один мир — типичный сценарий)
================================================================ */
static ParticlePool* g_pool=NULL;
static SpatialHash*  g_sh=NULL;

static void ensure_globals(void){
    if(!g_pool)g_pool=pool_create();
    if(!g_sh)  g_sh=sh_create();
}

/* ================================================================
   ИНТЕГРАЦИЯ
================================================================ */
static void integrate_euler(Particle* p,float dt){
    if(p->inv_mass<=0)return;
    Vec2 accel=vec2_mul(p->force,p->inv_mass);
    p->velocity=vec2_add(p->velocity,vec2_mul(accel,dt));
    /* Damping */
    float damp=1.f-p->damping*dt; if(damp<0)damp=0;
    p->velocity=vec2_mul(p->velocity,damp);
    p->prev_position=p->position;
    p->position=vec2_add(p->position,vec2_mul(p->velocity,dt));
    /* Угловой */
    p->angle+=p->angular_velocity*dt;
    p->angular_velocity*=(1.f-p->damping*dt*2);
}

static void integrate_verlet(Particle* p,float dt){
    if(p->inv_mass<=0)return;
    Vec2 accel=vec2_mul(p->force,p->inv_mass);
    Vec2 new_pos=vec2_add(vec2_mul(vec2_sub(p->position,p->prev_position),0.99f),vec2_mul(accel,dt*dt));
    new_pos=vec2_add(p->position,new_pos);
    p->velocity=vec2_mul(vec2_sub(new_pos,p->prev_position),1.f/(2.f*dt));
    p->prev_position=p->position;
    p->position=new_pos;
}

/* ================================================================
   СОЗДАНИЕ МИРА
================================================================ */
PhysicsWorld* physics_world_create(Rect bounds,float time_step){
    PhysicsWorld* w=(PhysicsWorld*)calloc(1,sizeof(PhysicsWorld));
    if(!w)return NULL;
    w->particle_capacity=POOL_SIZE;
    w->particles=(Particle**)malloc(sizeof(Particle*)*POOL_SIZE);
    w->particle_count=0;
    w->spring_capacity=4096;
    w->springs=(Spring**)malloc(sizeof(Spring*)*w->spring_capacity);
    w->spring_count=0;
    w->gravity=vec2(0,MATH_GRAVITY*100.f);
    w->time_step=time_step;
    w->iterations=4;
    w->substeps=1;
    w->use_verlet=false;
    w->bounds=bounds;
    w->bounds_enabled=true;
    w->wind=vec2_zero();
    w->air_resistance=0.001f;
    w->accumulated_time=0;
    w->total_time=0;
    return w;
}

void physics_world_destroy(PhysicsWorld* w){
    if(!w)return;
    for(int i=0;i<w->spring_count;i++)free(w->springs[i]);
    free(w->springs);
    free(w->particles);
    free(w);
}

void physics_world_reset(PhysicsWorld* w){
    if(!w)return;
    for(int i=0;i<w->spring_count;i++)free(w->springs[i]);
    w->spring_count=0;
    ensure_globals();
    for(int i=0;i<w->particle_count;i++){
        if(w->particles[i])pool_free(g_pool,w->particles[i]);
    }
    w->particle_count=0;
    w->dist_joint_count=0;
    w->rev_joint_count=0;
    w->trigger_count=0;
    w->force_field_count=0;
}

/* ================================================================
   ЧАСТИЦЫ
================================================================ */
Particle* particle_create(PhysicsWorld* w,Vec2 pos,float mass,float radius,ParticleType type){
    ensure_globals();
    Particle* p=pool_alloc(g_pool);
    if(!p){
        /* Реклаим мёртвых */
        for(int i=0;i<w->particle_count;i++){
            if(!w->particles[i]->active){
                pool_free(g_pool,w->particles[i]);
                w->particles[i]=w->particles[--w->particle_count];
                p=pool_alloc(g_pool); break;
            }
        }
        if(!p)return NULL;
    }
    memset(p,0,sizeof(Particle));
    p->position=pos; p->prev_position=pos;
    p->velocity=vec2_zero(); p->force=vec2_zero();
    p->mass=(mass<=0)?1.f:mass;
    p->inv_mass=(type==PARTICLE_TYPE_STATIC||type==PARTICLE_TYPE_TRIGGER)?0.f:1.f/p->mass;
    p->radius=radius; p->type=type; p->active=true;
    p->lifetime=-1.f; p->friction=0.5f; p->restitution=0.45f; p->damping=0.01f;
    p->layer=LAYER_DEFAULT; p->mask=LAYER_ALL;
    p->debug_color=0xFFFFFFFF;

    if(w->particle_count>=w->particle_capacity){
        w->particle_capacity*=2;
        w->particles=(Particle**)realloc(w->particles,sizeof(Particle*)*w->particle_capacity);
    }
    w->particles[w->particle_count++]=p;
    return p;
}

void particle_destroy(PhysicsWorld* w,Particle* p){ (void)w; if(p)p->active=false; }

void particle_apply_force(Particle* p,Vec2 f)  { p->force=vec2_add(p->force,f); }
void particle_apply_impulse(Particle* p,Vec2 im){ if(p->inv_mass>0)p->velocity=vec2_add(p->velocity,vec2_mul(im,p->inv_mass)); }

void particle_apply_force_at(Particle* p,Vec2 f,Vec2 point){
    p->force=vec2_add(p->force,f);
    Vec2 r=vec2_sub(point,p->position);
    p->angular_velocity+=vec2_cross(r,f)*p->inv_mass*0.01f; /* упрощённый момент */
}

void particle_set_position(Particle* p,Vec2 pos){ p->position=pos; p->prev_position=pos; p->velocity=vec2_zero(); }
void particle_set_layers(Particle* p,uint16_t layer,uint16_t mask){ p->layer=layer; p->mask=mask; }
void particle_set_lifetime(Particle* p,float s){ p->lifetime=s; }

void physics_explosion(PhysicsWorld* w,Vec2 center,float radius,float force){
    float r2=radius*radius;
    for(int i=0;i<w->particle_count;i++){
        Particle* p=w->particles[i];
        if(!p->active||p->inv_mass<=0)continue;
        Vec2 d=vec2_sub(p->position,center);
        float d2=vec2_length_sq(d);
        if(d2>r2)continue;
        float dist=sqrtf(d2);
        float atten=1.f-dist/radius;
        Vec2 n=dist>MATH_EPSILON?vec2_mul(d,1/dist):vec2(0,-1);
        particle_apply_impulse(p,vec2_mul(n,force*atten));
    }
}

/* ================================================================
   ПРУЖИНЫ
================================================================ */
static Spring* _spring_add(PhysicsWorld* w,Particle* a,Particle* b,float stiff,float damp){
    if(!a||!b)return NULL;
    if(w->spring_count>=w->spring_capacity){
        w->spring_capacity*=2;
        w->springs=(Spring**)realloc(w->springs,sizeof(Spring*)*w->spring_capacity);
    }
    Spring* s=(Spring*)malloc(sizeof(Spring));
    s->a=a; s->b=b;
    s->rest_distance=vec2_distance(a->position,b->position);
    s->stiffness=stiff; s->damping=damp; s->break_force=0;
    s->active=true; s->is_structural=true; s->is_shear=false; s->is_bend=false;
    w->springs[w->spring_count++]=s;
    return s;
}
Spring* spring_create(PhysicsWorld* w,Particle* a,Particle* b,float stiff,float damp){ return _spring_add(w,a,b,stiff,damp); }
Spring* spring_create_breakable(PhysicsWorld* w,Particle* a,Particle* b,float stiff,float damp,float bf){
    Spring* s=_spring_add(w,a,b,stiff,damp); if(s)s->break_force=bf; return s;
}
void spring_set_rest_length(Spring* s,float l){ if(s)s->rest_distance=l; }
void spring_destroy(PhysicsWorld* w,Spring* s){
    for(int i=0;i<w->spring_count;i++) if(w->springs[i]==s){
        w->springs[i]=w->springs[--w->spring_count]; free(s); return;
    }
}

/* ================================================================
   JOINTS
================================================================ */
DistanceJoint* dist_joint_create(PhysicsWorld* w,Particle* a,Particle* b,float mn,float mx){
    if(w->dist_joint_count>=MAX_DIST_JOINTS)return NULL;
    DistanceJoint* j=&w->dist_joints[w->dist_joint_count++];
    j->a=a; j->b=b; j->min_distance=mn; j->max_distance=mx; j->stiffness=1.f; j->active=true;
    return j;
}
RevoluteJoint* revolute_joint_create(PhysicsWorld* w,Vec2 pivot,Particle* body){
    if(w->rev_joint_count>=MAX_REV_JOINTS)return NULL;
    RevoluteJoint* j=&w->rev_joints[w->rev_joint_count++];
    j->pivot=pivot; j->body=body; j->anchor=NULL;
    j->angle_min=0; j->angle_max=0; j->motor_speed=0; j->motor_torque=0; j->active=true;
    return j;
}
void dist_joint_destroy(PhysicsWorld* w,DistanceJoint* j){ j->active=false; (void)w; }

/* ================================================================
   ТКАНЬ
================================================================ */
Cloth* cloth_create(PhysicsWorld* w,Vec2 tl,Vec2 br,int width,int height,float mpp){
    Cloth* cloth=(Cloth*)malloc(sizeof(Cloth));
    if(!cloth)return NULL;
    cloth->width=width; cloth->height=height;
    cloth->particles=(Particle**)malloc(sizeof(Particle*)*width*height);
    cloth->structural_stiffness=0.85f; cloth->shear_stiffness=0.55f;
    cloth->bend_stiffness=0.35f; cloth->damping=0.08f; cloth->tear_threshold=0;
    cloth->springs=NULL; cloth->spring_count=0;

    float sx=(br.x-tl.x)/(width-1), sy=(br.y-tl.y)/(height-1);
    for(int y=0;y<height;y++) for(int x=0;x<width;x++){
        Vec2 pos=vec2(tl.x+x*sx,tl.y+y*sy);
        cloth->particles[y*width+x]=particle_create(w,pos,mpp,2.f,PARTICLE_TYPE_CLOTH);
    }
    for(int y=0;y<height;y++) for(int x=0;x<width;x++){
        Particle* cur=cloth->particles[y*width+x];
        if(x<width-1) spring_create(w,cur,cloth->particles[y*width+x+1],cloth->structural_stiffness,cloth->damping);
        if(y<height-1)spring_create(w,cur,cloth->particles[(y+1)*width+x],cloth->structural_stiffness,cloth->damping);
        if(x<width-1&&y<height-1){ Spring* s=spring_create(w,cur,cloth->particles[(y+1)*width+x+1],cloth->shear_stiffness,cloth->damping); if(s)s->is_shear=true; }
        if(x>0&&y<height-1)      { Spring* s=spring_create(w,cur,cloth->particles[(y+1)*width+x-1],cloth->shear_stiffness,cloth->damping); if(s)s->is_shear=true; }
        if(x<width-2)             { Spring* s=spring_create(w,cur,cloth->particles[y*width+x+2],cloth->bend_stiffness*.3f,cloth->damping); if(s)s->is_bend=true; }
        if(y<height-2)            { Spring* s=spring_create(w,cur,cloth->particles[(y+2)*width+x],cloth->bend_stiffness*.3f,cloth->damping); if(s)s->is_bend=true; }
    }
    return cloth;
}
void cloth_destroy(PhysicsWorld* w,Cloth* cloth){
    if(!cloth)return;
    for(int i=0;i<cloth->width*cloth->height;i++) particle_destroy(w,cloth->particles[i]);
    free(cloth->particles); free(cloth);
}
void cloth_pin(Cloth* cloth,int x,int y){ if(x>=0&&x<cloth->width&&y>=0&&y<cloth->height){ Particle* p=cloth->particles[y*cloth->width+x]; p->inv_mass=0; p->type=PARTICLE_TYPE_STATIC; } }
void cloth_unpin(Cloth* cloth,int x,int y){ if(x>=0&&x<cloth->width&&y>=0&&y<cloth->height){ Particle* p=cloth->particles[y*cloth->width+x]; p->inv_mass=1.f/p->mass; p->type=PARTICLE_TYPE_CLOTH; } }
void cloth_apply_wind(Cloth* cloth,Vec2 wind){ for(int i=0;i<cloth->width*cloth->height;i++){ Particle* p=cloth->particles[i]; if(p->inv_mass>0)particle_apply_force(p,wind); } }
void cloth_set_tear_threshold(Cloth* cloth,float t){ cloth->tear_threshold=t; }
void cloth_tear_at(PhysicsWorld* w,Cloth* cloth,Vec2 pos,float radius){
    for(int i=0;i<cloth->width*cloth->height;i++){
        Particle* p=cloth->particles[i];
        if(vec2_distance(p->position,pos)<radius){ p->inv_mass=0; p->type=PARTICLE_TYPE_STATIC; (void)w; }
    }
}

/* ================================================================
   МЯГКОЕ ТЕЛО
================================================================ */
SoftBody* softbody_create_circle(PhysicsWorld* w,Vec2 center,float radius,int segments,float mass_total,float stiff){
    SoftBody* body=(SoftBody*)malloc(sizeof(SoftBody));
    if(!body)return NULL;
    body->particle_count=segments+1;
    body->particles=(Particle**)malloc(sizeof(Particle*)*body->particle_count);
    float mpp=mass_total/body->particle_count;
    body->particles[0]=particle_create(w,center,mpp,radius*.3f,PARTICLE_TYPE_SOFT_BODY);
    float step=MATH_TAU/segments;
    for(int i=0;i<segments;i++){
        float a=i*step;
        body->particles[i+1]=particle_create(w,vec2(center.x+radius*cosf(a),center.y+radius*sinf(a)),mpp,radius*.2f,PARTICLE_TYPE_SOFT_BODY);
    }
    for(int i=0;i<segments;i++){
        int next=(i+1)%segments;
        spring_create(w,body->particles[i+1],body->particles[next+1],stiff,.1f);
        spring_create(w,body->particles[0],body->particles[i+1],stiff*.5f,.1f);
    }
    for(int i=0;i<segments;i++) for(int j=i+2;j<segments&&j-i<segments-1;j++){
        spring_create(w,body->particles[i+1],body->particles[j+1],stiff*.3f,.1f);
    }
    body->spring_count=0; body->springs=NULL;
    body->volume=MATH_PI*radius*radius; body->pressure=0; body->bulk_modulus=120.f;
    return body;
}

SoftBody* softbody_create_rectangle(PhysicsWorld* w,Vec2 mn,Vec2 mx,int sx,int sy,float mass_total,float stiff){
    SoftBody* body=(SoftBody*)malloc(sizeof(SoftBody));
    if(!body)return NULL;
    body->particle_count=sx*sy;
    body->particles=(Particle**)malloc(sizeof(Particle*)*body->particle_count);
    float mpp=mass_total/body->particle_count;
    float dx=(mx.x-mn.x)/(sx-1), dy=(mx.y-mn.y)/(sy-1);
    for(int y=0;y<sy;y++) for(int x=0;x<sx;x++){
        body->particles[y*sx+x]=particle_create(w,vec2(mn.x+x*dx,mn.y+y*dy),mpp,4.f,PARTICLE_TYPE_SOFT_BODY);
    }
    for(int y=0;y<sy;y++) for(int x=0;x<sx;x++){
        Particle* cur=body->particles[y*sx+x];
        if(x<sx-1)spring_create(w,cur,body->particles[y*sx+x+1],stiff,.1f);
        if(y<sy-1)spring_create(w,cur,body->particles[(y+1)*sx+x],stiff,.1f);
        if(x<sx-1&&y<sy-1){ spring_create(w,cur,body->particles[(y+1)*sx+x+1],stiff*.6f,.1f); spring_create(w,body->particles[y*sx+x+1],body->particles[(y+1)*sx+x],stiff*.6f,.1f); }
    }
    body->spring_count=0; body->springs=NULL;
    body->volume=(mx.x-mn.x)*(mx.y-mn.y); body->pressure=0; body->bulk_modulus=120.f;
    return body;
}

void softbody_destroy(PhysicsWorld* w,SoftBody* body){
    if(!body)return;
    for(int i=0;i<body->particle_count;i++)particle_destroy(w,body->particles[i]);
    free(body->particles); free(body);
}
Vec2 softbody_center(SoftBody* body){
    Vec2 c=vec2_zero(); int n=0;
    for(int i=0;i<body->particle_count;i++) if(body->particles[i]->active){ c=vec2_add(c,body->particles[i]->position); n++; }
    return n>0?vec2_mul(c,1.f/n):vec2_zero();
}
void softbody_apply_force(SoftBody* body,Vec2 f){ for(int i=0;i<body->particle_count;i++) if(body->particles[i]->inv_mass>0)particle_apply_force(body->particles[i],f); }

/* Pressure-based soft body inflation */
void softbody_apply_pressure(SoftBody* body,PhysicsWorld* w,float dt){
    if(!body||body->bulk_modulus<=0)return;
    /* Вычисляем текущий "объём" как периметр */
    float vol=0;
    for(int i=0;i<w->spring_count;i++){
        Spring* s=w->springs[i];
        if(!s->active)return;
        float len=vec2_distance(s->a->position,s->b->position);
        vol+=len;
    }
    float pressure=body->bulk_modulus*(body->volume-vol)*dt;
    for(int i=0;i<body->particle_count;i++){
        Particle* p=body->particles[i];
        if(p->inv_mass<=0)continue;
        Vec2 n=vec2_normalize(vec2_sub(p->position,softbody_center(body)));
        particle_apply_force(p,vec2_mul(n,pressure));
    }
}

/* ================================================================
   TRIGGER ZONES
================================================================ */
TriggerZone* trigger_zone_create(PhysicsWorld* w,AABB bounds){
    if(w->trigger_count>=MAX_TRIGGER_ZONES)return NULL;
    TriggerZone* t=&w->triggers[w->trigger_count++];
    memset(t,0,sizeof(TriggerZone));
    t->bounds=bounds; t->active=true; t->filter_layer=LAYER_ALL;
    return t;
}
void trigger_zone_destroy(PhysicsWorld* w,TriggerZone* t){ t->active=false; (void)w; }
bool trigger_zone_has_particle(TriggerZone* t,Particle* p){
    for(int i=0;i<t->count;i++) if(t->particles_inside[i]==(int)(p-(Particle*)NULL))return true; return false;
}

/* ================================================================
   FORCE FIELDS
================================================================ */
ForceField* force_field_create(PhysicsWorld* w,ForceFieldType type,Vec2 pos,float radius,float strength){
    if(w->force_field_count>=MAX_FORCE_FIELDS)return NULL;
    ForceField* f=&w->force_fields[w->force_field_count++];
    memset(f,0,sizeof(ForceField));
    f->type=type; f->position=pos; f->radius=radius; f->strength=strength;
    f->falloff=1.f; f->active=true; f->layer_mask=LAYER_ALL;
    return f;
}
void force_field_destroy(PhysicsWorld* w,ForceField* f){ f->active=false; (void)w; }
void force_field_set_direction(ForceField* f,Vec2 dir){ f->direction=vec2_normalize(dir); }

/* ================================================================
   RAYCASTING
================================================================ */
RaycastHit physics_raycast(PhysicsWorld* w,Vec2 origin,Vec2 dir,float max_dist,uint16_t mask){
    RaycastHit result={0};
    result.distance=max_dist;
    Vec2 nd=vec2_normalize(dir);
    for(int i=0;i<w->particle_count;i++){
        Particle* p=w->particles[i];
        if(!p->active)continue;
        if(!(p->layer&mask))continue;
        /* Ray-sphere */
        Vec2 oc=vec2_sub(origin,p->position);
        float b=vec2_dot(oc,nd);
        float c=vec2_dot(oc,oc)-p->radius*p->radius;
        float disc=b*b-c;
        if(disc<0)continue;
        float t=-b-sqrtf(disc);
        if(t<0)t=-b+sqrtf(disc);
        if(t<0||t>result.distance)continue;
        result.hit=true;
        result.distance=t;
        result.point=vec2_add(origin,vec2_mul(nd,t));
        result.normal=vec2_normalize(vec2_sub(result.point,p->position));
        result.particle=p;
    }
    return result;
}

int physics_overlap_circle(PhysicsWorld* w,Vec2 center,float radius,uint16_t mask,Particle** out,int max_out){
    int n=0;
    for(int i=0;i<w->particle_count&&n<max_out;i++){
        Particle* p=w->particles[i];
        if(!p->active)continue;
        if(!(p->layer&mask))continue;
        float d2=vec2_distance_sq(center,p->position);
        if(d2<=(radius+p->radius)*(radius+p->radius)) out[n++]=p;
    }
    return n;
}

int physics_overlap_rect(PhysicsWorld* w,AABB bounds,uint16_t mask,Particle** out,int max_out){
    int n=0;
    for(int i=0;i<w->particle_count&&n<max_out;i++){
        Particle* p=w->particles[i];
        if(!p->active)continue;
        if(!(p->layer&mask))continue;
        if(aabb_contains(bounds,p->position)) out[n++]=p;
    }
    return n;
}

/* ================================================================
   ПРУЖИНЫ UPDATE
================================================================ */
void physics_world_update_springs(PhysicsWorld* w){
    for(int i=w->spring_count-1;i>=0;i--){
        Spring* s=w->springs[i];
        if(!s->active){ w->springs[i]=w->springs[--w->spring_count]; free(s); continue; }
        Particle* a=s->a; Particle* b=s->b;
        if(!a->active||!b->active){ s->active=false; continue; }

        Vec2 delta=vec2_sub(b->position,a->position);
        float dist=vec2_length(delta);
        if(dist<MATH_EPSILON)continue;

        Vec2 dir=vec2_mul(delta,1.f/dist);
        float stretch=dist-s->rest_distance;
        float fm=s->stiffness*stretch;
        Vec2 rv=vec2_sub(b->velocity,a->velocity);
        float dm=s->damping*vec2_dot(rv,dir);
        Vec2 force=vec2_mul(dir,fm+dm);

        /* Проверка разрыва */
        if(s->break_force>0&&fabsf(fm)>s->break_force){
            if(w->on_spring_break)w->on_spring_break(s);
            s->active=false;
            continue;
        }

        if(a->inv_mass>0)a->force=vec2_add(a->force,force);
        if(b->inv_mass>0)b->force=vec2_sub(b->force,force);
    }
}

/* ================================================================
   JOINTS UPDATE
================================================================ */
void physics_world_update_joints(PhysicsWorld* w){
    /* Distance joints */
    for(int i=0;i<w->dist_joint_count;i++){
        DistanceJoint* j=&w->dist_joints[i];
        if(!j->active||!j->a->active||!j->b->active)continue;
        Vec2 delta=vec2_sub(j->b->position,j->a->position);
        float dist=vec2_length(delta);
        if(dist<MATH_EPSILON)continue;
        Vec2 dir=vec2_mul(delta,1/dist);
        float corr=0;
        if(dist>j->max_distance)corr=dist-j->max_distance;
        else if(dist<j->min_distance&&j->min_distance>0)corr=dist-j->min_distance;
        if(fabsf(corr)<MATH_EPSILON)continue;
        float ta=j->a->inv_mass,tb=j->b->inv_mass,total=ta+tb;
        if(total<MATH_EPSILON)continue;
        Vec2 c=vec2_mul(dir,corr*j->stiffness);
        if(j->a->inv_mass>0)j->a->position=vec2_add(j->a->position,vec2_mul(c, ta/total));
        if(j->b->inv_mass>0)j->b->position=vec2_sub(j->b->position,vec2_mul(c, tb/total));
    }
    /* Revolute joints */
    for(int i=0;i<w->rev_joint_count;i++){
        RevoluteJoint* j=&w->rev_joints[i];
        if(!j->active||!j->body->active)continue;
        /* Простой pivot constraint: тянем тело к pivot */
        float ta=j->body->inv_mass; if(ta<=0)continue;
        Vec2 delta=vec2_sub(j->pivot,j->body->position);
        /* Только угловое ограничение если задано */
        (void)delta; /* корректное position constraint требует второй точки */
    }
}

/* ================================================================
   FORCE FIELDS UPDATE
================================================================ */
void physics_world_update_force_fields(PhysicsWorld* w,float dt){
    for(int fi=0;fi<w->force_field_count;fi++){
        ForceField* ff=&w->force_fields[fi];
        if(!ff->active)continue;
        ff->time_phase+=dt;

        for(int pi=0;pi<w->particle_count;pi++){
            Particle* p=w->particles[pi];
            if(!p->active||p->inv_mass<=0)continue;
            if(!(p->layer&ff->layer_mask))continue;

            Vec2 d=vec2_sub(p->position,ff->position);
            float dist=vec2_length(d);

            /* Ограничение радиусом */
            if(ff->radius>0&&dist>ff->radius)continue;

            /* Затухание */
            float atten=ff->strength;
            if(ff->radius>0&&dist>0){
                float t=1.f-dist/ff->radius;
                if(ff->falloff==1)atten*=t;
                else if(ff->falloff==2)atten*=t*t;
            }

            Vec2 force=vec2_zero();
            switch(ff->type){
                case FORCE_FIELD_ATTRACTOR:
                    force=vec2_mul(dist>MATH_EPSILON?vec2_mul(d,-1/dist):vec2_zero(),atten); break;
                case FORCE_FIELD_REPELLER:
                    force=vec2_mul(dist>MATH_EPSILON?vec2_mul(d,1/dist):vec2_zero(),atten); break;
                case FORCE_FIELD_VORTEX:
                    force=vec2_mul(vec2_perp(d),atten/(dist+1.f)); break;
                case FORCE_FIELD_DIRECTIONAL:
                    force=vec2_mul(ff->direction,atten); break;
                case FORCE_FIELD_DRAG:
                    force=vec2_mul(p->velocity,-atten*p->mass); break;
                case FORCE_FIELD_WIND_ZONE:
                    force=vec2_mul(ff->direction,atten*(0.8f+0.2f*fast_sin(ff->time_phase*3+p->position.x*0.1f))); break;
                case FORCE_FIELD_TURBULENCE:{
                    float nx=simplex2(p->position.x*0.05f,ff->time_phase);
                    float ny=simplex2(p->position.y*0.05f,ff->time_phase+100);
                    force=vec2_mul(vec2(nx*2-1,ny*2-1),atten); break;
                }
            }
            particle_apply_force(p,force);
        }
    }
}

/* ================================================================
   TRIGGER ZONES UPDATE
================================================================ */
void physics_world_update_triggers(PhysicsWorld* w){
    for(int ti=0;ti<w->trigger_count;ti++){
        TriggerZone* t=&w->triggers[ti];
        if(!t->active)continue;

        int new_inside[64], new_count=0;
        for(int pi=0;pi<w->particle_count&&new_count<64;pi++){
            Particle* p=w->particles[pi];
            if(!p->active)continue;
            if(!(p->layer&t->filter_layer))continue;
            if(!aabb_contains(t->bounds,p->position))continue;
            new_inside[new_count++]=pi;
        }

        /* Проверяем enter */
        for(int a=0;a<new_count;a++){
            bool was_inside=false;
            for(int b=0;b<t->count;b++) if(t->particles_inside[b]==new_inside[a]){was_inside=true;break;}
            if(!was_inside&&t->on_enter)t->on_enter(w->particles[new_inside[a]],t->user_data);
        }
        /* Проверяем exit */
        for(int a=0;a<t->count;a++){
            bool still_inside=false;
            for(int b=0;b<new_count;b++) if(new_inside[b]==t->particles_inside[a]){still_inside=true;break;}
            if(!still_inside&&t->on_exit)t->on_exit(w->particles[t->particles_inside[a]],t->user_data);
        }
        memcpy(t->particles_inside,new_inside,new_count*sizeof(int));
        t->count=new_count;
    }
}

/* ================================================================
   КОЛЛИЗИИ (Spatial Hash)
================================================================ */
void physics_world_check_collisions(PhysicsWorld* w){
    ensure_globals();
    sh_clear(g_sh);
    for(int i=0;i<w->particle_count;i++){
        Particle* p=w->particles[i];
        if(p->active)sh_insert(g_sh,i,p->position.x,p->position.y);
    }
    w->stat_collision_pairs=0;
    for(int i=0;i<w->particle_count;i++){
        Particle* a=w->particles[i];
        if(!a->active||a->type==PARTICLE_TYPE_TRIGGER)continue;
        int cx=(int)floorf(a->position.x/SH_CELL_SIZE);
        int cy=(int)floorf(a->position.y/SH_CELL_SIZE);
        for(int dy=-1;dy<=1;dy++) for(int dx=-1;dx<=1;dx++){
            SHCell* cell=&g_sh->cells[sh_hash(cx+dx,cy+dy)];
            for(int k=0;k<cell->count;k++){
                int j=cell->idx[k];
                if(j<=i)continue;
                Particle* b=w->particles[j];
                if(!b->active||b->type==PARTICLE_TYPE_TRIGGER)continue;
                if(!(a->mask&b->layer)||!(b->mask&a->layer))continue;
                if(a->inv_mass==0&&b->inv_mass==0)continue;
                float md=a->radius+b->radius;
                float dsq=vec2_distance_sq(a->position,b->position);
                if(dsq<md*md){
                    w->stat_collision_pairs++;
                    if(w->on_collision){
                        Vec2 n=vec2_normalize(vec2_sub(b->position,a->position));
                        float imp=0;
                        w->on_collision(a,b,vec2_add(a->position,vec2_mul(n,a->radius)),n,imp);
                    }
                }
            }
        }
    }
}

void physics_world_resolve_collisions(PhysicsWorld* w){
    ensure_globals();
    for(int i=0;i<w->particle_count;i++){
        Particle* a=w->particles[i];
        if(!a->active||a->type==PARTICLE_TYPE_TRIGGER)continue;
        int cx=(int)floorf(a->position.x/SH_CELL_SIZE);
        int cy=(int)floorf(a->position.y/SH_CELL_SIZE);
        for(int dy=-1;dy<=1;dy++) for(int dx=-1;dx<=1;dx++){
            SHCell* cell=&g_sh->cells[sh_hash(cx+dx,cy+dy)];
            for(int k=0;k<cell->count;k++){
                int j=cell->idx[k];
                if(j<=i)continue;
                Particle* b=w->particles[j];
                if(!b->active||b->type==PARTICLE_TYPE_TRIGGER)continue;
                if(!(a->mask&b->layer)||!(b->mask&a->layer))continue;
                if(a->inv_mass==0&&b->inv_mass==0)continue;
                float md=a->radius+b->radius;
                Vec2 delta=vec2_sub(b->position,a->position);
                float dsq=vec2_length_sq(delta);
                if(dsq>=md*md||dsq<MATH_EPSILON)continue;
                float dist=sqrtf(dsq);
                Vec2 normal=vec2_mul(delta,1/dist);
                float overlap=md-dist;
                float ta=a->inv_mass,tb=b->inv_mass,total=ta+tb;
                if(total<MATH_EPSILON)continue;
                Vec2 corr=vec2_mul(normal,overlap*0.5f);
                if(a->inv_mass>0)a->position=vec2_sub(a->position,vec2_mul(corr,ta/total));
                if(b->inv_mass>0)b->position=vec2_add(b->position,vec2_mul(corr,tb/total));
                Vec2 rv=vec2_sub(b->velocity,a->velocity);
                float van=vec2_dot(rv,normal);
                if(van<0){
                    float rest=fminf((a->restitution+b->restitution)*.5f,1.f);
                    float imp=-(1+rest)*van/total;
                    Vec2 iv=vec2_mul(normal,imp);
                    if(a->inv_mass>0)a->velocity=vec2_sub(a->velocity,vec2_mul(iv,ta));
                    if(b->inv_mass>0)b->velocity=vec2_add(b->velocity,vec2_mul(iv,tb));
                    /* Трение */
                    Vec2 tv=vec2_sub(rv,vec2_mul(normal,van));
                    float tvl=vec2_length(tv);
                    if(tvl>MATH_EPSILON){
                        float frict_imp=fminf((a->friction+b->friction)*.5f*fabsf(imp),tvl)/total;
                        Vec2 tv_dir=vec2_mul(tv,1/tvl);
                        if(a->inv_mass>0)a->velocity=vec2_add(a->velocity,vec2_mul(tv_dir,frict_imp*ta));
                        if(b->inv_mass>0)b->velocity=vec2_sub(b->velocity,vec2_mul(tv_dir,frict_imp*tb));
                    }
                }
            }
        }
    }
}

/* ================================================================
   ГЛАВНЫЙ UPDATE
================================================================ */
void physics_world_update(PhysicsWorld* w){
    if(!w)return;
    ensure_globals();

    int substeps=w->substeps>0?w->substeps:1;
    float sub_dt=w->time_step/substeps;

    for(int step=0;step<substeps;step++){
        float dt=sub_dt;

        /* Сброс сил */
        w->stat_active_particles=0;
        for(int i=0;i<w->particle_count;i++){
            Particle* p=w->particles[i];
            if(!p->active)continue;
            p->force=vec2_zero();
            w->stat_active_particles++;
        }

        /* Гравитация + ветер + сопротивление воздуха */
        for(int i=0;i<w->particle_count;i++){
            Particle* p=w->particles[i];
            if(!p->active||p->inv_mass<=0)continue;
            p->force=vec2_add(p->force,vec2_mul(w->gravity,p->mass));
            p->force=vec2_sub(p->force,vec2_mul(p->velocity,w->air_resistance*p->mass));
            p->force=vec2_add(p->force,w->wind);
        }

        /* Силовые поля */
        physics_world_update_force_fields(w,dt);

        /* Пружины */
        physics_world_update_springs(w);

        /* Интеграция */
        for(int i=0;i<w->particle_count;i++){
            Particle* p=w->particles[i];
            if(!p->active||p->inv_mass<=0)continue;
            if(w->use_verlet)integrate_verlet(p,dt);
            else integrate_euler(p,dt);
        }

        /* Joints */
        for(int iter=0;iter<w->iterations;iter++){
            physics_world_update_joints(w);
        }

        /* Коллизии */
        physics_world_check_collisions(w);
        physics_world_resolve_collisions(w);

        /* Границы мира */
        if(w->bounds_enabled){
            for(int i=0;i<w->particle_count;i++){
                Particle* p=w->particles[i];
                if(!p->active||p->inv_mass<=0)continue;
                if(p->position.x-p->radius<w->bounds.x)        { p->position.x=w->bounds.x+p->radius;           p->velocity.x*=-p->restitution; }
                if(p->position.x+p->radius>w->bounds.x+w->bounds.w){ p->position.x=w->bounds.x+w->bounds.w-p->radius; p->velocity.x*=-p->restitution; }
                if(p->position.y-p->radius<w->bounds.y)        { p->position.y=w->bounds.y+p->radius;           p->velocity.y*=-p->restitution; }
                if(p->position.y+p->radius>w->bounds.y+w->bounds.h){ p->position.y=w->bounds.y+w->bounds.h-p->radius; p->velocity.y*=-p->restitution; }
            }
        }
    }

    /* Триггеры */
    physics_world_update_triggers(w);

    /* Lifetime + уборка мёртвых (swap O(1)) */
    float dt=w->time_step;
    for(int i=w->particle_count-1;i>=0;i--){
        Particle* p=w->particles[i];
        if(p->lifetime>0){ p->lifetime-=dt; if(p->lifetime<=0)p->active=false; }
        if(!p->active){
            /* Удаляем связи */
            for(int j=w->spring_count-1;j>=0;j--){
                Spring* s=w->springs[j];
                if(s->a==p||s->b==p){ w->springs[j]=w->springs[--w->spring_count]; free(s); }
            }
            if(w->on_particle_death)w->on_particle_death(p);
            pool_free(g_pool,p);
            w->particles[i]=w->particles[--w->particle_count];
        }
    }

    w->total_time+=dt;
}

/* ================================================================
   УТИЛИТЫ
================================================================ */
void physics_world_set_gravity(PhysicsWorld* w,Vec2 g){ w->gravity=g; }
void physics_world_set_substeps(PhysicsWorld* w,int n){ w->substeps=n>0?n:1; }
void physics_world_set_iterations(PhysicsWorld* w,int n){ w->iterations=n>0?n:1; }

void physics_world_add_force_region(PhysicsWorld* w,Rect region,Vec2 force){
    for(int i=0;i<w->particle_count;i++){
        Particle* p=w->particles[i];
        if(p->active&&rect_contains_point(region,p->position))particle_apply_force(p,force);
    }
}

void physics_world_clear_forces(PhysicsWorld* w){
    for(int i=0;i<w->particle_count;i++) w->particles[i]->force=vec2_zero();
}

float physics_world_kinetic_energy(PhysicsWorld* w){
    float ke=0;
    for(int i=0;i<w->particle_count;i++){
        Particle* p=w->particles[i];
        if(p->active&&p->inv_mass>0) ke+=0.5f*p->mass*vec2_length_sq(p->velocity);
    }
    return ke;
}

Vec2 physics_world_center_of_mass(PhysicsWorld* w){
    Vec2 cm=vec2_zero(); float total_mass=0;
    for(int i=0;i<w->particle_count;i++){
        Particle* p=w->particles[i];
        if(p->active&&p->inv_mass>0){ cm=vec2_add(cm,vec2_mul(p->position,p->mass)); total_mass+=p->mass; }
    }
    return total_mass>MATH_EPSILON?vec2_mul(cm,1/total_mass):vec2_zero();
}
