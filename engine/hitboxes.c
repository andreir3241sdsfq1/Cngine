/* hitboxes.c — Cngine v2
 * LGPL v3 | Chikatilo / sooborn / SGLauncher
 */
#include "hitboxes.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

/* ================================================================
   ВНУТРЕННИЕ УТИЛИТЫ
================================================================ */
static Vec2 _rotate(Vec2 v, float a) {
    float c = cosf(a), s = sinf(a);
    return vec2(v.x*c - v.y*s, v.x*s + v.y*c);
}

static float _clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static Vec2 _world_center(const Hitbox* hb, Vec2 wp, float wr) {
    Vec2 off = _rotate(hb->offset, wr);
    return vec2(wp.x + off.x, wp.y + off.y);
}

/* ================================================================
   ФАБРИКА
================================================================ */
Hitbox* hb_create_aabb(float half_w, float half_h) {
    Hitbox* hb = (Hitbox*)calloc(1, sizeof(Hitbox));
    hb->shape = HB_AABB;
    hb->aabb.w = half_w;
    hb->aabb.h = half_h;
    hb->active = true;
    hb->layer  = 0xFFFF;
    hb->mask   = 0xFFFF;
    return hb;
}

Hitbox* hb_create_circle(float radius) {
    Hitbox* hb = (Hitbox*)calloc(1, sizeof(Hitbox));
    hb->shape = HB_CIRCLE;
    hb->circle.radius = radius;
    hb->active = true;
    hb->layer  = 0xFFFF;
    hb->mask   = 0xFFFF;
    return hb;
}

Hitbox* hb_create_obb(float half_w, float half_h) {
    Hitbox* hb = (Hitbox*)calloc(1, sizeof(Hitbox));
    hb->shape = HB_OBB;
    hb->obb.w = half_w;
    hb->obb.h = half_h;
    hb->active = true;
    hb->layer  = 0xFFFF;
    hb->mask   = 0xFFFF;
    return hb;
}

Hitbox* hb_create_capsule(float radius, float half_len) {
    Hitbox* hb = (Hitbox*)calloc(1, sizeof(Hitbox));
    hb->shape = HB_CAPSULE;
    hb->capsule.radius = radius;
    hb->capsule.len    = half_len;
    hb->active = true;
    hb->layer  = 0xFFFF;
    hb->mask   = 0xFFFF;
    return hb;
}

Hitbox* hb_create_polygon(Vec2* verts, int count) {
    if (count > HB_MAX_POLY_VERTS) count = HB_MAX_POLY_VERTS;
    Hitbox* hb = (Hitbox*)calloc(1, sizeof(Hitbox));
    hb->shape = HB_POLYGON;
    memcpy(hb->polygon.verts, verts, sizeof(Vec2)*count);
    hb->polygon.count = count;
    hb->active = true;
    hb->layer  = 0xFFFF;
    hb->mask   = 0xFFFF;
    return hb;
}

Hitbox* hb_create_composite(void) {
    Hitbox* hb = (Hitbox*)calloc(1, sizeof(Hitbox));
    hb->shape  = (HitboxShape)-1; /* composite sentinel */
    hb->active = true;
    hb->layer  = 0xFFFF;
    hb->mask   = 0xFFFF;
    return hb;
}

void hb_composite_add(Hitbox* parent, Hitbox* child) {
    if (!parent || !child || parent->child_count >= HB_MAX_CHILDREN) return;
    parent->children[parent->child_count++] = child;
}

void hb_destroy(Hitbox* hb) {
    if (!hb) return;
    /* не удаляем children рекурсивно — управление памятью на стороне пользователя */
    free(hb);
}

/* ================================================================
   ТРАНСФОРМАЦИЯ
================================================================ */
Vec2 hb_world_center(const Hitbox* hb, Vec2 wp, float wr) {
    return _world_center(hb, wp, wr);
}

AABB hb_world_aabb(const Hitbox* hb, Vec2 wp, float wr) {
    Vec2 c = _world_center(hb, wp, wr);
    float rot = wr + hb->rotation;
    float hw, hh;
    switch(hb->shape) {
        case HB_AABB:
            return (AABB){{c.x - hb->aabb.w, c.y - hb->aabb.h},
                          {c.x + hb->aabb.w, c.y + hb->aabb.h}};
        case HB_CIRCLE:
            hw = hb->circle.radius; hh = hw;
            return (AABB){{c.x-hw, c.y-hh},{c.x+hw, c.y+hh}};
        case HB_OBB: {
            float ax = fabsf(cosf(rot)*hb->obb.w) + fabsf(sinf(rot)*hb->obb.h);
            float ay = fabsf(sinf(rot)*hb->obb.w) + fabsf(cosf(rot)*hb->obb.h);
            return (AABB){{c.x-ax, c.y-ay},{c.x+ax, c.y+ay}};
        }
        case HB_CAPSULE: {
            Vec2 ax = _rotate(vec2(0, hb->capsule.len), rot);
            float r  = hb->capsule.radius;
            float mnx = c.x - fabsf(ax.x) - r, mny = c.y - fabsf(ax.y) - r;
            float mxx = c.x + fabsf(ax.x) + r, mxy = c.y + fabsf(ax.y) + r;
            return (AABB){{mnx, mny},{mxx, mxy}};
        }
        case HB_POLYGON: {
            AABB ab = {{1e9f, 1e9f},{-1e9f, -1e9f}};
            for(int i=0;i<hb->polygon.count;i++){
                Vec2 v = _rotate(hb->polygon.verts[i], rot);
                v.x += c.x; v.y += c.y;
                if(v.x<ab.min.x)ab.min.x=v.x; if(v.y<ab.min.y)ab.min.y=v.y;
                if(v.x>ab.max.x)ab.max.x=v.x; if(v.y>ab.max.y)ab.max.y=v.y;
            }
            return ab;
        }
        default: {
            /* composite: union of children */
            AABB ab = {{1e9f, 1e9f},{-1e9f, -1e9f}};
            for(int i=0;i<hb->child_count;i++){
                AABB ch = hb_world_aabb(hb->children[i], wp, wr);
                if(ch.min.x<ab.min.x)ab.min.x=ch.min.x;
                if(ch.min.y<ab.min.y)ab.min.y=ch.min.y;
                if(ch.max.x>ab.max.x)ab.max.x=ch.max.x;
                if(ch.max.y>ab.max.y)ab.max.y=ch.max.y;
            }
            return ab;
        }
    }
}

/* ================================================================
   ПРИМИТИВНЫЕ ТЕСТЫ
================================================================ */
/* Circle vs Circle */
static HitResult _test_cc(Vec2 ca, float ra, Vec2 cb, float rb) {
    HitResult r = {0};
    float dx = cb.x-ca.x, dy = cb.y-ca.y;
    float dist2 = dx*dx+dy*dy;
    float sum = ra+rb;
    if(dist2 >= sum*sum) return r;
    float dist = sqrtf(dist2);
    r.hit   = true;
    r.depth = sum - dist;
    if(dist < 1e-6f) { r.normal = vec2(1,0); }
    else { r.normal = vec2(dx/dist, dy/dist); }
    r.point = vec2(ca.x + r.normal.x*ra, ca.y + r.normal.y*ra);
    return r;
}

/* AABB vs AABB */
static HitResult _test_aa(Vec2 ca, float hw_a, float hh_a,
                           Vec2 cb, float hw_b, float hh_b) {
    HitResult r = {0};
    float dx = cb.x-ca.x, dy = cb.y-ca.y;
    float ox = hw_a+hw_b - fabsf(dx);
    float oy = hh_a+hh_b - fabsf(dy);
    if(ox<=0||oy<=0) return r;
    r.hit = true;
    if(ox < oy){ r.depth=ox; r.normal=vec2(dx<0?1.f:-1.f, 0); }
    else        { r.depth=oy; r.normal=vec2(0, dy<0?1.f:-1.f); }
    r.point = vec2(ca.x + r.normal.x*hw_a, ca.y + r.normal.y*hh_a);
    return r;
}

/* Circle vs AABB */
static HitResult _test_ca(Vec2 cc, float cr, Vec2 bc, float hw, float hh) {
    HitResult r = {0};
    float cx = _clampf(cc.x, bc.x-hw, bc.x+hw);
    float cy = _clampf(cc.y, bc.y-hh, bc.y+hh);
    float dx = cc.x-cx, dy = cc.y-cy;
    float d2 = dx*dx+dy*dy;
    if(d2 >= cr*cr) return r;
    float d = sqrtf(d2);
    r.hit = true;
    r.depth = cr - d;
    if(d < 1e-6f){ r.normal=vec2(0,-1); }
    else { r.normal=vec2(dx/d,dy/d); }
    r.point = vec2(cx, cy);
    return r;
}

/* SAT для OBB vs OBB */
static HitResult _test_oo(Vec2 ca, float wa, float ha, float ra,
                           Vec2 cb, float wb, float hb, float rb) {
    HitResult res = {0};
    /* axes: 4 осей SAT */
    float axes_angle[4] = { ra, ra+(float)M_PI_2, rb, rb+(float)M_PI_2 };
    float best_depth = 1e9f;
    Vec2 best_normal = vec2(0,0);

    for(int i=0;i<4;i++){
        Vec2 ax = vec2(cosf(axes_angle[i]), sinf(axes_angle[i]));
        /* project A */
        float pa = fabsf(cosf(ra)*wa * ax.x + sinf(ra)*wa * ax.y)
                 + fabsf(-sinf(ra)*ha * ax.x + cosf(ra)*ha * ax.y);
        float pb = fabsf(cosf(rb)*wb * ax.x + sinf(rb)*wb * ax.y)
                 + fabsf(-sinf(rb)*hb * ax.x + cosf(rb)*hb * ax.y);
        float dc = fabsf((cb.x-ca.x)*ax.x + (cb.y-ca.y)*ax.y);
        float overlap = pa + pb - dc;
        if(overlap <= 0) return res;
        if(overlap < best_depth) {
            best_depth = overlap;
            best_normal = ax;
        }
    }
    float sign = (cb.x-ca.x)*best_normal.x+(cb.y-ca.y)*best_normal.y;
    if(sign < 0) best_normal = vec2(-best_normal.x,-best_normal.y);
    res.hit = true; res.depth = best_depth; res.normal = best_normal;
    res.point = vec2(ca.x + best_normal.x*(wa+ha)*0.5f,
                     ca.y + best_normal.y*(wa+ha)*0.5f);
    return res;
}

/* Closest point on segment */
static Vec2 _closest_on_seg(Vec2 p, Vec2 a, Vec2 b) {
    float dx=b.x-a.x,dy=b.y-a.y;
    float len2=dx*dx+dy*dy;
    if(len2<1e-9f) return a;
    float t=((p.x-a.x)*dx+(p.y-a.y)*dy)/len2;
    t=_clampf(t,0,1);
    return vec2(a.x+t*dx, a.y+t*dy);
}

/* Capsule vs Capsule */
static HitResult _test_caps(Vec2 ca, float ra, float la, float rota,
                             Vec2 cb, float rb, float lb, float rotb) {
    Vec2 ax=_rotate(vec2(0,la),rota), bx=_rotate(vec2(0,lb),rotb);
    Vec2 a0=vec2(ca.x-ax.x,ca.y-ax.y), a1=vec2(ca.x+ax.x,ca.y+ax.y);
    Vec2 b0=vec2(cb.x-bx.x,cb.y-bx.y), b1=vec2(cb.x+bx.x,cb.y+bx.y);
    Vec2 mid_a = vec2((a0.x+a1.x)*0.5f,(a0.y+a1.y)*0.5f);
    Vec2 cp = _closest_on_seg(mid_a, b0, b1);
    Vec2 ca2 = _closest_on_seg(cp, a0, a1);
    return _test_cc(ca2, ra, cp, rb);
}

/* ================================================================
   ГЛАВНЫЙ ТЕСТ
================================================================ */
HitResult hb_test(const Hitbox* a, Vec2 pa, float ra,
                  const Hitbox* b, Vec2 pb, float rb) {
    if(!a||!b||!a->active||!b->active) return (HitResult){0};
    if(!(a->layer & b->mask) && !(b->layer & a->mask)) return (HitResult){0};

    /* Composite: проверяем все комбинации дочерних */
    if(a->child_count>0) {
        for(int i=0;i<a->child_count;i++){
            HitResult r = hb_test(a->children[i],pa,ra,b,pb,rb);
            if(r.hit) return r;
        }
        return (HitResult){0};
    }
    if(b->child_count>0) {
        for(int i=0;i<b->child_count;i++){
            HitResult r = hb_test(a,pa,ra,b->children[i],pb,rb);
            if(r.hit) return r;
        }
        return (HitResult){0};
    }

    Vec2 ca = _world_center(a, pa, ra);
    Vec2 cb = _world_center(b, pb, rb);
    float ra_tot = ra + a->rotation;
    float rb_tot = rb + b->rotation;

    /* Circle vs X */
    if(a->shape==HB_CIRCLE && b->shape==HB_CIRCLE)
        return _test_cc(ca, a->circle.radius, cb, b->circle.radius);
    if(a->shape==HB_CIRCLE && b->shape==HB_AABB)
        return _test_ca(ca, a->circle.radius, cb, b->aabb.w, b->aabb.h);
    if(a->shape==HB_AABB && b->shape==HB_CIRCLE) {
        HitResult r = _test_ca(cb, b->circle.radius, ca, a->aabb.w, a->aabb.h);
        r.normal = vec2(-r.normal.x,-r.normal.y); return r;
    }

    /* AABB vs AABB */
    if(a->shape==HB_AABB && b->shape==HB_AABB)
        return _test_aa(ca, a->aabb.w, a->aabb.h, cb, b->aabb.w, b->aabb.h);

    /* OBB vs OBB */
    if(a->shape==HB_OBB && b->shape==HB_OBB)
        return _test_oo(ca, a->obb.w, a->obb.h, ra_tot,
                        cb, b->obb.w, b->obb.h, rb_tot);
    if(a->shape==HB_OBB && b->shape==HB_AABB)
        return _test_oo(ca, a->obb.w, a->obb.h, ra_tot,
                        cb, b->aabb.w, b->aabb.h, 0);
    if(a->shape==HB_AABB && b->shape==HB_OBB)
        return _test_oo(ca, a->aabb.w, a->aabb.h, 0,
                        cb, b->obb.w, b->obb.h, rb_tot);

    /* Circle vs OBB: approximate via AABB of OBB (good enough for most cases) */
    if(a->shape==HB_CIRCLE && b->shape==HB_OBB)
        return _test_ca(ca, a->circle.radius, cb, b->obb.w, b->obb.h);
    if(a->shape==HB_OBB && b->shape==HB_CIRCLE) {
        HitResult r = _test_ca(cb, b->circle.radius, ca, a->obb.w, a->obb.h);
        r.normal = vec2(-r.normal.x,-r.normal.y); return r;
    }

    /* Capsule vs Capsule */
    if(a->shape==HB_CAPSULE && b->shape==HB_CAPSULE)
        return _test_caps(ca, a->capsule.radius, a->capsule.len, ra_tot,
                          cb, b->capsule.radius, b->capsule.len, rb_tot);

    /* Capsule vs Circle */
    if(a->shape==HB_CAPSULE && b->shape==HB_CIRCLE) {
        Vec2 ax=_rotate(vec2(0,a->capsule.len),ra_tot);
        Vec2 cp = _closest_on_seg(cb, vec2(ca.x-ax.x,ca.y-ax.y), vec2(ca.x+ax.x,ca.y+ax.y));
        return _test_cc(cp, a->capsule.radius, cb, b->circle.radius);
    }
    if(a->shape==HB_CIRCLE && b->shape==HB_CAPSULE) {
        Vec2 bx=_rotate(vec2(0,b->capsule.len),rb_tot);
        Vec2 cp = _closest_on_seg(ca, vec2(cb.x-bx.x,cb.y-bx.y), vec2(cb.x+bx.x,cb.y+bx.y));
        HitResult r = _test_cc(cp, b->capsule.radius, ca, a->circle.radius);
        r.normal = vec2(-r.normal.x,-r.normal.y); return r;
    }

    /* Polygon vs Circle (approximate) */
    if(a->shape==HB_POLYGON && b->shape==HB_CIRCLE) {
        /* find closest edge */
        HitResult best = {0}; best.depth = -1e9f;
        int n = a->polygon.count;
        for(int i=0;i<n;i++){
            Vec2 v0 = _rotate(a->polygon.verts[i], ra_tot);
            v0.x+=ca.x; v0.y+=ca.y;
            Vec2 v1 = _rotate(a->polygon.verts[(i+1)%n], ra_tot);
            v1.x+=ca.x; v1.y+=ca.y;
            Vec2 cp = _closest_on_seg(cb, v0, v1);
            HitResult r = _test_cc(cp, 0.01f, cb, b->circle.radius);
            if(r.hit && r.depth > best.depth) best = r;
        }
        return best;
    }

    /* Fallback: AABB vs AABB */
    AABB aa = hb_world_aabb(a, pa, ra);
    AABB ab = hb_world_aabb(b, pb, rb);
    Vec2 caa = vec2((aa.min.x+aa.max.x)*0.5f,(aa.min.y+aa.max.y)*0.5f);
    Vec2 cab = vec2((ab.min.x+ab.max.x)*0.5f,(ab.min.y+ab.max.y)*0.5f);
    return _test_aa(caa,(aa.max.x-aa.min.x)*0.5f,(aa.max.y-aa.min.y)*0.5f,
                    cab,(ab.max.x-ab.min.x)*0.5f,(ab.max.y-ab.min.y)*0.5f);
}

/* ================================================================
   CONTAINS POINT
================================================================ */
bool hb_contains_point(const Hitbox* hb, Vec2 wp, float wr, Vec2 pt) {
    if(!hb||!hb->active) return false;
    Vec2 c = _world_center(hb, wp, wr);
    float rot = wr + hb->rotation;
    switch(hb->shape){
        case HB_AABB:
            return pt.x>=c.x-hb->aabb.w && pt.x<=c.x+hb->aabb.w &&
                   pt.y>=c.y-hb->aabb.h && pt.y<=c.y+hb->aabb.h;
        case HB_CIRCLE: {
            float dx=pt.x-c.x,dy=pt.y-c.y;
            return dx*dx+dy*dy <= hb->circle.radius*hb->circle.radius;
        }
        case HB_OBB: {
            /* rotate point into local space */
            float dx=pt.x-c.x,dy=pt.y-c.y;
            float lx= dx*cosf(rot)+dy*sinf(rot);
            float ly=-dx*sinf(rot)+dy*cosf(rot);
            return fabsf(lx)<=hb->obb.w && fabsf(ly)<=hb->obb.h;
        }
        case HB_CAPSULE: {
            Vec2 ax=_rotate(vec2(0,hb->capsule.len),rot);
            Vec2 a0=vec2(c.x-ax.x,c.y-ax.y),a1=vec2(c.x+ax.x,c.y+ax.y);
            Vec2 cp=_closest_on_seg(pt,a0,a1);
            float dx=pt.x-cp.x,dy=pt.y-cp.y;
            return dx*dx+dy*dy<=hb->capsule.radius*hb->capsule.radius;
        }
        case HB_POLYGON: {
            /* ray casting */
            int n=hb->polygon.count; bool inside=false;
            Vec2 verts[HB_MAX_POLY_VERTS];
            for(int i=0;i<n;i++){
                verts[i]=_rotate(hb->polygon.verts[i],rot);
                verts[i].x+=c.x; verts[i].y+=c.y;
            }
            for(int i=0,j=n-1;i<n;j=i++){
                if(((verts[i].y>pt.y)!=(verts[j].y>pt.y))&&
                    (pt.x<(verts[j].x-verts[i].x)*(pt.y-verts[i].y)/(verts[j].y-verts[i].y)+verts[i].x))
                    inside=!inside;
            }
            return inside;
        }
        default:
            for(int i=0;i<hb->child_count;i++)
                if(hb_contains_point(hb->children[i],wp,wr,pt)) return true;
            return false;
    }
}

/* ================================================================
   RAYCAST
================================================================ */
bool hb_raycast(const Hitbox* hb, Vec2 wp, float wr,
                Vec2 ro, Vec2 rd, float max_dist, float* out_t) {
    if(!hb||!hb->active) return false;
    AABB ab = hb_world_aabb(hb, wp, wr);
    /* Slab method for AABB */
    float inv_x = (fabsf(rd.x)>1e-9f) ? 1.f/rd.x : 1e9f;
    float inv_y = (fabsf(rd.y)>1e-9f) ? 1.f/rd.y : 1e9f;
    float tx1=(ab.min.x-ro.x)*inv_x, tx2=(ab.max.x-ro.x)*inv_x;
    float ty1=(ab.min.y-ro.y)*inv_y, ty2=(ab.max.y-ro.y)*inv_y;
    float tmin=fmaxf(fminf(tx1,tx2),fminf(ty1,ty2));
    float tmax=fminf(fmaxf(tx1,tx2),fmaxf(ty1,ty2));
    if(tmax<0||tmin>tmax||tmin>max_dist) return false;
    if(out_t) *out_t = tmin<0?tmax:tmin;
    return true;
}

/* ================================================================
   SWEEP
================================================================ */
HitResult hb_sweep(const Hitbox* a, Vec2 pa, float ra, Vec2 delta,
                   const Hitbox* b, Vec2 pb, float rb) {
    /* Binary search along motion */
    HitResult best={0}; best.t=1.f;
    int steps=8;
    for(int i=1;i<=steps;i++){
        float t=(float)i/steps;
        Vec2 pos=vec2(pa.x+delta.x*t, pa.y+delta.y*t);
        HitResult r=hb_test(a,pos,ra,b,pb,rb);
        if(r.hit){ r.t=t; best=r; break; }
    }
    return best;
}

/* ================================================================
   МЕНЕДЖЕР
================================================================ */
HitboxManager* hbm_create(void) {
    HitboxManager* m=(HitboxManager*)calloc(1,sizeof(HitboxManager));
    return m;
}

void hbm_destroy(HitboxManager* m) { free(m); }

int hbm_add(HitboxManager* m, Hitbox* hb, Vec2 pos, float rot) {
    if(!m||m->count>=HBM_MAX) return -1;
    int idx=m->count++;
    m->boxes[idx]=hb;
    m->positions[idx]=pos;
    m->rotations[idx]=rot;
    return idx;
}

void hbm_remove(HitboxManager* m, int idx) {
    if(!m||idx<0||idx>=m->count) return;
    int last=--m->count;
    m->boxes[idx]=m->boxes[last];
    m->positions[idx]=m->positions[last];
    m->rotations[idx]=m->rotations[last];
    m->ids[idx]=m->ids[last];
}

void hbm_update_transform(HitboxManager* m, int idx, Vec2 pos, float rot) {
    if(!m||idx<0||idx>=m->count) return;
    m->positions[idx]=pos;
    m->rotations[idx]=rot;
}

void hbm_process(HitboxManager* m) {
    if(!m) return;
    for(int i=0;i<m->count;i++) {
        for(int j=i+1;j<m->count;j++) {
            Hitbox* a=m->boxes[i];
            Hitbox* b=m->boxes[j];
            if(!a||!b) continue;
            HitResult r=hb_test(a,m->positions[i],m->rotations[i],
                                 b,m->positions[j],m->rotations[j]);
            if(r.hit){
                if(a->on_collision) a->on_collision(a,b,r,a->callback_ud);
                HitResult ri=r; ri.normal=vec2(-r.normal.x,-r.normal.y);
                if(b->on_collision) b->on_collision(b,a,ri,b->callback_ud);
            }
        }
    }
}

int hbm_query_hitbox(HitboxManager* m, const Hitbox* q, Vec2 qp, float qr,
                     int* out_i, HitResult* out_r, int max_out) {
    int found=0;
    for(int i=0;i<m->count&&found<max_out;i++){
        HitResult r=hb_test(q,qp,qr,m->boxes[i],m->positions[i],m->rotations[i]);
        if(r.hit){ if(out_i)out_i[found]=i; if(out_r)out_r[found]=r; found++; }
    }
    return found;
}

int hbm_query_aabb(HitboxManager* m, AABB region, int* out_i, int max_out) {
    int found=0;
    for(int i=0;i<m->count&&found<max_out;i++){
        AABB ab=hb_world_aabb(m->boxes[i],m->positions[i],m->rotations[i]);
        if(ab.max.x>=region.min.x&&ab.min.x<=region.max.x&&
           ab.max.y>=region.min.y&&ab.min.y<=region.max.y){
            if(out_i)out_i[found]=i; found++;
        }
    }
    return found;
}

int hbm_raycast(HitboxManager* m, Vec2 origin, Vec2 dir, float max_dist,
                uint16_t layer_mask, int* out_idx, float* out_t) {
    float best_t=max_dist; int best_i=-1;
    for(int i=0;i<m->count;i++){
        Hitbox* hb=m->boxes[i];
        if(!(hb->layer & layer_mask)) continue;
        float t;
        if(hb_raycast(hb,m->positions[i],m->rotations[i],origin,dir,best_t,&t)){
            if(t<best_t){ best_t=t; best_i=i; }
        }
    }
    if(out_idx)*out_idx=best_i;
    if(out_t)*out_t=best_t;
    return best_i>=0?1:0;
}

/* ================================================================
   DEBUG RENDER
================================================================ */
void hb_debug_draw(Renderer* r, const Hitbox* hb, Vec2 wp, float wr,
                   Color ac, Color tc, bool filled) {
    if(!hb||!r) return;
    Color col = hb->is_trigger ? tc : ac;
    Vec2 c = _world_center(hb, wp, wr);
    float rot = wr + hb->rotation;

    /* Composite */
    if(hb->child_count>0){
        for(int i=0;i<hb->child_count;i++)
            hb_debug_draw(r,hb->children[i],wp,wr,ac,tc,filled);
        return;
    }

    switch(hb->shape){
        case HB_AABB:
            renderer_draw_rect(r,
                rect(c.x-hb->aabb.w, c.y-hb->aabb.h,
                     hb->aabb.w*2, hb->aabb.h*2), col, filled);
            break;
        case HB_CIRCLE:
            renderer_draw_circle(r, c, hb->circle.radius, col, filled);
            break;
        case HB_OBB:
            renderer_draw_obb(r, (OBB){c, vec2(hb->obb.w,hb->obb.h), rot}, col);
            break;
        case HB_CAPSULE: {
            Vec2 ax=_rotate(vec2(0,hb->capsule.len),rot);
            Vec2 a0=vec2(c.x-ax.x,c.y-ax.y),a1=vec2(c.x+ax.x,c.y+ax.y);
            renderer_draw_capsule(r, a0, a1, hb->capsule.radius, col);
            break;
        }
        case HB_POLYGON: {
            int n=hb->polygon.count;
            Vec2 verts[HB_MAX_POLY_VERTS];
            for(int i=0;i<n;i++){
                verts[i]=_rotate(hb->polygon.verts[i],rot);
                verts[i].x+=c.x; verts[i].y+=c.y;
            }
            renderer_draw_polygon(r,verts,n,col,filled);
            break;
        }
        default: break;
    }
}

void hbm_debug_draw_all(Renderer* r, const HitboxManager* m,
                        Color ac, Color tc) {
    if(!m||!r) return;
    for(int i=0;i<m->count;i++)
        hb_debug_draw(r,m->boxes[i],m->positions[i],m->rotations[i],ac,tc,false);
}

/* ================================================================
   УТИЛИТЫ
================================================================ */
Hitbox* hb_from_particle_radius(float radius) {
    return hb_create_circle(radius);
}

void hb_scale(Hitbox* hb, float sx, float sy) {
    if(!hb) return;
    switch(hb->shape){
        case HB_AABB:   hb->aabb.w*=sx; hb->aabb.h*=sy; break;
        case HB_CIRCLE: hb->circle.radius*=(sx+sy)*0.5f; break;
        case HB_OBB:    hb->obb.w*=sx; hb->obb.h*=sy; break;
        case HB_CAPSULE: hb->capsule.radius*=(sx+sy)*0.5f; hb->capsule.len*=sy; break;
        case HB_POLYGON:
            for(int i=0;i<hb->polygon.count;i++){
                hb->polygon.verts[i].x*=sx;
                hb->polygon.verts[i].y*=sy;
            }
            break;
        default:
            for(int i=0;i<hb->child_count;i++) hb_scale(hb->children[i],sx,sy);
    }
}
