/* engine_math.h — Cngine v2
 * LGPL v3 | Chikatilo / sooborn / SGLauncher
 *
 * НОВОЕ v2:
 *   Vec4, Quaternion, AABB, OBB
 *   Bezier (квадратичный + кубический), Catmull-Rom
 *   24 easing-функции (ease-in, ease-out, elastic, bounce, back...)
 *   Mat4 (TRS, ortho, perspective, look_at, inverse)
 *   HSV <-> RGB цветовые пространства
 *   Улучшенный RNG (xorshift64): нормальное распределение, circle
 *   Worley noise, Simplex 2D
 *   Bit-tricks: fast_rsqrt, fast_sin/cos
 *   move_towards, remap, wrap, ping_pong, snap, clamp01
 */
#ifndef ENGINE_MATH_H
#define ENGINE_MATH_H

#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

/* ================================================================
   КОНСТАНТЫ
================================================================ */
#define MATH_PI          3.14159265358979323846f
#define MATH_TAU         6.28318530717958647692f
#define MATH_PI_HALF     1.57079632679489661923f
#define MATH_EPSILON     1e-6f
#define MATH_GRAVITY     9.81f
#define MATH_DEG2RAD     (MATH_PI / 180.0f)
#define MATH_RAD2DEG     (180.0f / MATH_PI)
#define MATH_SQRT2       1.41421356237f
#define MATH_INF         (1e30f)

/* ================================================================
   БАЗОВЫЕ ТИПЫ
================================================================ */
typedef struct { float x, y; }         Vec2;
typedef struct { float x, y, z; }      Vec3;
typedef struct { float x, y, z, w; }   Vec4;
typedef struct { float x, y, z, w; }   Quat;
typedef struct { float m[2][2]; }       Mat2;
typedef struct { float m[3][3]; }       Mat3;
typedef struct { float m[4][4]; }       Mat4; /* row-major */
typedef struct { float x,y,w,h; }       Rect;
typedef struct { Vec2 start, end; }     Line;
typedef struct { Vec2 center; float radius; } Circle;
typedef struct { Vec2 min, max; }       AABB;
typedef struct { Vec2 center; Vec2 half_extents; float angle; } OBB;
typedef struct { Vec2 vertices[8]; int count; } Polygon;

/* ================================================================
   VEC2 — inline для скорости
================================================================ */
static inline Vec2  vec2(float x,float y)      { Vec2 v={x,y};   return v; }
static inline Vec2  vec2_zero(void)             { Vec2 v={0,0};   return v; }
static inline Vec2  vec2_one(void)              { Vec2 v={1,1};   return v; }
static inline Vec2  vec2_up(void)               { Vec2 v={0,-1};  return v; }
static inline Vec2  vec2_right(void)            { Vec2 v={1,0};   return v; }
static inline Vec2  vec2_add(Vec2 a,Vec2 b)     { return vec2(a.x+b.x,a.y+b.y); }
static inline Vec2  vec2_sub(Vec2 a,Vec2 b)     { return vec2(a.x-b.x,a.y-b.y); }
static inline Vec2  vec2_mul(Vec2 v,float s)    { return vec2(v.x*s,v.y*s); }
static inline Vec2  vec2_div(Vec2 v,float s)    { if(s==0)return vec2_zero(); return vec2(v.x/s,v.y/s); }
static inline Vec2  vec2_neg(Vec2 v)            { return vec2(-v.x,-v.y); }
static inline Vec2  vec2_hadamard(Vec2 a,Vec2 b){ return vec2(a.x*b.x,a.y*b.y); }
static inline Vec2  vec2_abs(Vec2 v)            { return vec2(fabsf(v.x),fabsf(v.y)); }
static inline Vec2  vec2_min2(Vec2 a,Vec2 b)    { return vec2(a.x<b.x?a.x:b.x,a.y<b.y?a.y:b.y); }
static inline Vec2  vec2_max2(Vec2 a,Vec2 b)    { return vec2(a.x>b.x?a.x:b.x,a.y>b.y?a.y:b.y); }
static inline float vec2_dot(Vec2 a,Vec2 b)     { return a.x*b.x+a.y*b.y; }
static inline float vec2_cross(Vec2 a,Vec2 b)   { return a.x*b.y-a.y*b.x; }
static inline float vec2_length_sq(Vec2 v)      { return v.x*v.x+v.y*v.y; }
static inline float vec2_length(Vec2 v)         { return sqrtf(v.x*v.x+v.y*v.y); }
static inline float vec2_distance_sq(Vec2 a,Vec2 b){ return vec2_length_sq(vec2_sub(a,b)); }
static inline float vec2_distance(Vec2 a,Vec2 b)   { return sqrtf(vec2_distance_sq(a,b)); }
static inline Vec2  vec2_normalize(Vec2 v){ float l=vec2_length(v); if(l<MATH_EPSILON)return vec2_zero(); return vec2_div(v,l); }
static inline Vec2  vec2_perp(Vec2 v)           { return vec2(-v.y,v.x); }
static inline Vec2  vec2_perp_cw(Vec2 v)        { return vec2(v.y,-v.x); }
static inline Vec2  vec2_lerp(Vec2 a,Vec2 b,float t){ return vec2(a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t); }
static inline Vec2  vec2_reflect(Vec2 v,Vec2 n) { return vec2_sub(v,vec2_mul(n,2.f*vec2_dot(v,n))); }
static inline Vec2  vec2_project(Vec2 v,Vec2 o) { float l=vec2_length_sq(o); if(l<MATH_EPSILON)return vec2_zero(); return vec2_mul(o,vec2_dot(v,o)/l); }
static inline Vec2  vec2_rotate(Vec2 v,float a) { float ca=cosf(a),sa=sinf(a); return vec2(v.x*ca-v.y*sa,v.x*sa+v.y*ca); }
static inline Vec2  vec2_clamp_magnitude(Vec2 v,float m){ float l=vec2_length(v); if(l>m&&l>MATH_EPSILON)return vec2_mul(v,m/l); return v; }
static inline float vec2_angle(Vec2 v)          { return atan2f(v.y,v.x); }
static inline float vec2_angle_between(Vec2 a,Vec2 b){ return acosf(fmaxf(-1.f,fminf(1.f,vec2_dot(vec2_normalize(a),vec2_normalize(b))))); }
static inline Vec2  vec2_from_angle(float a)    { return vec2(cosf(a),sinf(a)); }
static inline bool  vec2_equal(Vec2 a,Vec2 b)   { return fabsf(a.x-b.x)<MATH_EPSILON&&fabsf(a.y-b.y)<MATH_EPSILON; }
static inline Vec2  vec2_move_towards(Vec2 cur,Vec2 tgt,float md){
    Vec2 d=vec2_sub(tgt,cur); float l=vec2_length(d);
    if(l<=md||l<MATH_EPSILON)return tgt;
    return vec2_add(cur,vec2_mul(d,md/l));
}
static inline Vec2  vec2_smooth_damp(Vec2 cur,Vec2 tgt,Vec2* vel,float time,float dt){
    float o=2.f/time; float x=o*dt; float d=1.f/(1.f+x+.48f*x*x+.235f*x*x*x);
    Vec2 cd=vec2_sub(cur,tgt);
    Vec2 t=vec2_mul(vec2_add(cd,vec2_mul(*vel,time)),dt);
    *vel=vec2_mul(vec2_sub(*vel,vec2_mul(t,o)),d);
    return vec2_add(tgt,vec2_mul(vec2_add(cd,t),d));
}

/* ================================================================
   VEC3
================================================================ */
static inline Vec3  vec3(float x,float y,float z){ Vec3 v={x,y,z}; return v; }
static inline Vec3  vec3_zero(void) { Vec3 v={0,0,0};  return v; }
static inline Vec3  vec3_one(void)  { Vec3 v={1,1,1};  return v; }
static inline Vec3  vec3_up(void)   { Vec3 v={0,1,0};  return v; }
static inline Vec3  vec3_add(Vec3 a,Vec3 b){ return vec3(a.x+b.x,a.y+b.y,a.z+b.z); }
static inline Vec3  vec3_sub(Vec3 a,Vec3 b){ return vec3(a.x-b.x,a.y-b.y,a.z-b.z); }
static inline Vec3  vec3_mul(Vec3 v,float s){ return vec3(v.x*s,v.y*s,v.z*s); }
static inline Vec3  vec3_div(Vec3 v,float s){ if(s==0)return vec3_zero(); return vec3(v.x/s,v.y/s,v.z/s); }
static inline Vec3  vec3_neg(Vec3 v){ return vec3(-v.x,-v.y,-v.z); }
static inline Vec3  vec3_hadamard(Vec3 a,Vec3 b){ return vec3(a.x*b.x,a.y*b.y,a.z*b.z); }
static inline float vec3_dot(Vec3 a,Vec3 b)     { return a.x*b.x+a.y*b.y+a.z*b.z; }
static inline float vec3_length_sq(Vec3 v)      { return v.x*v.x+v.y*v.y+v.z*v.z; }
static inline float vec3_length(Vec3 v)         { return sqrtf(vec3_length_sq(v)); }
static inline float vec3_distance(Vec3 a,Vec3 b){ return vec3_length(vec3_sub(a,b)); }
static inline Vec3  vec3_normalize(Vec3 v){ float l=vec3_length(v); if(l<MATH_EPSILON)return vec3_zero(); return vec3_div(v,l); }
static inline Vec3  vec3_cross(Vec3 a,Vec3 b){ return vec3(a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x); }
static inline Vec3  vec3_lerp(Vec3 a,Vec3 b,float t){ return vec3(a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t); }
static inline Vec3  vec3_reflect(Vec3 v,Vec3 n){ return vec3_sub(v,vec3_mul(n,2.f*vec3_dot(v,n))); }
static inline Vec2  vec3_xy(Vec3 v){ return vec2(v.x,v.y); }
static inline Vec3  vec2_to_vec3(Vec2 v,float z){ return vec3(v.x,v.y,z); }

/* ================================================================
   VEC4
================================================================ */
static inline Vec4  vec4(float x,float y,float z,float w){ Vec4 v={x,y,z,w}; return v; }
static inline Vec4  vec4_zero(void){ Vec4 v={0,0,0,0}; return v; }
static inline Vec4  vec4_add(Vec4 a,Vec4 b){ return vec4(a.x+b.x,a.y+b.y,a.z+b.z,a.w+b.w); }
static inline Vec4  vec4_mul(Vec4 v,float s){ return vec4(v.x*s,v.y*s,v.z*s,v.w*s); }
static inline float vec4_dot(Vec4 a,Vec4 b){ return a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w; }
static inline Vec4  vec4_lerp(Vec4 a,Vec4 b,float t){ return vec4(a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t,a.w+(b.w-a.w)*t); }
static inline Vec3  vec4_xyz(Vec4 v){ return vec3(v.x,v.y,v.z); }

/* ================================================================
   КВАТЕРНИОН
================================================================ */
static inline Quat  quat(float x,float y,float z,float w){ Quat q={x,y,z,w}; return q; }
static inline Quat  quat_identity(void){ return quat(0,0,0,1); }
static inline float quat_length(Quat q){ return sqrtf(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w); }
static inline Quat  quat_normalize(Quat q){ float l=quat_length(q); if(l<MATH_EPSILON)return quat_identity(); return quat(q.x/l,q.y/l,q.z/l,q.w/l); }
static inline Quat  quat_conjugate(Quat q){ return quat(-q.x,-q.y,-q.z,q.w); }
static inline Quat  quat_mul(Quat a,Quat b){ return quat(a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z); }
static inline Vec3  quat_rotate_vec3(Quat q,Vec3 v){ Vec3 u=vec3(q.x,q.y,q.z); float s=q.w; return vec3_add(vec3_add(vec3_mul(u,2.f*vec3_dot(u,v)),vec3_mul(v,s*s-vec3_dot(u,u))),vec3_mul(vec3_cross(u,v),2.f*s)); }
Quat quat_from_axis_angle(Vec3 axis,float angle);
Quat quat_from_euler(float pitch,float yaw,float roll);
Quat quat_slerp(Quat a,Quat b,float t);
Vec3 quat_to_euler(Quat q);

/* ================================================================
   МАТРИЦЫ
================================================================ */
Mat2 mat2_identity(void);
Mat2 mat2_rotation(float angle);
Mat2 mat2_scale(float sx,float sy);
Vec2 mat2_mul_vec2(Mat2 m,Vec2 v);

Mat3 mat3_identity(void);
Mat3 mat3_translation(Vec2 t);
Mat3 mat3_rotation(float angle);
Mat3 mat3_scale(float sx,float sy);
Mat3 mat3_mul(Mat3 a,Mat3 b);
Vec2 mat3_mul_vec2(Mat3 m,Vec2 v);
Mat3 mat3_inverse(Mat3 m);
Mat3 mat3_transpose(Mat3 m);

Mat4 mat4_identity(void);
Mat4 mat4_translation(Vec3 t);
Mat4 mat4_rotation_x(float a);
Mat4 mat4_rotation_y(float a);
Mat4 mat4_rotation_z(float a);
Mat4 mat4_rotation_quat(Quat q);
Mat4 mat4_scale3(Vec3 s);
Mat4 mat4_mul(Mat4 a,Mat4 b);
Vec4 mat4_mul_vec4(Mat4 m,Vec4 v);
Vec3 mat4_mul_point(Mat4 m,Vec3 v);  /* w=1 */
Vec3 mat4_mul_dir(Mat4 m,Vec3 v);    /* w=0 */
Mat4 mat4_trs(Vec3 t,Quat r,Vec3 s); /* Transform */
Mat4 mat4_ortho(float l,float r,float b,float top,float near,float far);
Mat4 mat4_perspective(float fov_y,float aspect,float near,float far);
Mat4 mat4_look_at(Vec3 eye,Vec3 target,Vec3 up);
Mat4 mat4_inverse(Mat4 m);
Mat4 mat4_transpose(Mat4 m);

/* ================================================================
   ГЕОМЕТРИЯ
================================================================ */
static inline Rect  rect(float x,float y,float w,float h){ Rect r={x,y,w,h}; return r; }
static inline AABB  aabb(float x0,float y0,float x1,float y1){ AABB a={{x0,y0},{x1,y1}}; return a; }
static inline AABB  aabb_from_center(Vec2 c,Vec2 h){ return aabb(c.x-h.x,c.y-h.y,c.x+h.x,c.y+h.y); }
static inline Vec2  aabb_center(AABB a){ return vec2((a.min.x+a.max.x)*.5f,(a.min.y+a.max.y)*.5f); }
static inline Vec2  aabb_half(AABB a)  { return vec2((a.max.x-a.min.x)*.5f,(a.max.y-a.min.y)*.5f); }
static inline AABB  aabb_expand(AABB a,float s){ return aabb(a.min.x-s,a.min.y-s,a.max.x+s,a.max.y+s); }
static inline AABB  aabb_union(AABB a,AABB b){ return aabb(fminf(a.min.x,b.min.x),fminf(a.min.y,b.min.y),fmaxf(a.max.x,b.max.x),fmaxf(a.max.y,b.max.y)); }
static inline bool  aabb_contains(AABB a,Vec2 p){ return p.x>=a.min.x&&p.x<=a.max.x&&p.y>=a.min.y&&p.y<=a.max.y; }
static inline bool  aabb_overlaps(AABB a,AABB b){ return a.min.x<b.max.x&&a.max.x>b.min.x&&a.min.y<b.max.y&&a.max.y>b.min.y; }

bool rect_contains_point(Rect r,Vec2 p);
bool circle_contains_point(Circle c,Vec2 p);
bool line_intersect_line(Line a,Line b,Vec2* intersection);
bool line_intersect_circle(Line l,Circle c,Vec2* intersection);
bool circle_intersect_circle(Circle a,Circle b,Vec2* penetration);
bool obb_contains_point(OBB o,Vec2 p);
bool obb_overlap_obb(OBB a,OBB b,Vec2* mtv);

static inline Vec2 closest_point_on_segment(Vec2 p,Vec2 a,Vec2 b){
    Vec2 ab=vec2_sub(b,a); float l=vec2_dot(ab,ab);
    if(l<MATH_EPSILON)return a;
    float t=fmaxf(0,fminf(1,vec2_dot(vec2_sub(p,a),ab)/l));
    return vec2_add(a,vec2_mul(ab,t));
}
static inline float distance_point_to_segment(Vec2 p,Vec2 a,Vec2 b){ return vec2_distance(p,closest_point_on_segment(p,a,b)); }

/* ================================================================
   BEZIER / СПЛАЙНЫ
================================================================ */
static inline Vec2 bezier2(Vec2 p0,Vec2 p1,Vec2 p2,float t){ float mt=1-t; return vec2_add(vec2_add(vec2_mul(p0,mt*mt),vec2_mul(p1,2*mt*t)),vec2_mul(p2,t*t)); }
static inline Vec2 bezier2_tangent(Vec2 p0,Vec2 p1,Vec2 p2,float t){ return vec2_add(vec2_mul(vec2_sub(p1,p0),2*(1-t)),vec2_mul(vec2_sub(p2,p1),2*t)); }
static inline Vec2 bezier3(Vec2 p0,Vec2 p1,Vec2 p2,Vec2 p3,float t){ float mt=1-t,mt2=mt*mt,t2=t*t; return vec2_add(vec2_add(vec2_add(vec2_mul(p0,mt2*mt),vec2_mul(p1,3*mt2*t)),vec2_mul(p2,3*mt*t2)),vec2_mul(p3,t2*t)); }
static inline Vec2 bezier3_tangent(Vec2 p0,Vec2 p1,Vec2 p2,Vec2 p3,float t){ float mt=1-t; return vec2_add(vec2_add(vec2_mul(vec2_sub(p1,p0),3*mt*mt),vec2_mul(vec2_sub(p2,p1),6*mt*t)),vec2_mul(vec2_sub(p3,p2),3*t*t)); }
static inline Vec2 catmull_rom(Vec2 p0,Vec2 p1,Vec2 p2,Vec2 p3,float t){ float t2=t*t,t3=t2*t; return vec2(.5f*(2*p1.x+(-p0.x+p2.x)*t+(2*p0.x-5*p1.x+4*p2.x-p3.x)*t2+(-p0.x+3*p1.x-3*p2.x+p3.x)*t3),.5f*(2*p1.y+(-p0.y+p2.y)*t+(2*p0.y-5*p1.y+4*p2.y-p3.y)*t2+(-p0.y+3*p1.y-3*p2.y+p3.y)*t3)); }

/* ================================================================
   EASING (24 функции, t ∈ [0,1])
================================================================ */
static inline float ease_linear(float t)      { return t; }
static inline float ease_in_quad(float t)     { return t*t; }
static inline float ease_out_quad(float t)    { return t*(2-t); }
static inline float ease_inout_quad(float t)  { return t<.5f?2*t*t:t*(4-2*t)-1; }
static inline float ease_in_cubic(float t)    { return t*t*t; }
static inline float ease_out_cubic(float t)   { float s=1-t; return 1-s*s*s; }
static inline float ease_inout_cubic(float t) { return t<.5f?4*t*t*t:1-4*(1-t)*(1-t)*(1-t); }
static inline float ease_in_quart(float t)    { return t*t*t*t; }
static inline float ease_out_quart(float t)   { float s=t-1; return 1-s*s*s*s; }
static inline float ease_inout_quart(float t) { float s=1-2*t; return t<.5f?8*t*t*t*t:1-.5f*s*s*s*s; }
static inline float ease_in_sine(float t)     { return 1-cosf(t*MATH_PI_HALF); }
static inline float ease_out_sine(float t)    { return sinf(t*MATH_PI_HALF); }
static inline float ease_inout_sine(float t)  { return .5f*(1-cosf(t*MATH_PI)); }
static inline float ease_in_expo(float t)     { return t==0?0:powf(2,10*(t-1)); }
static inline float ease_out_expo(float t)    { return t==1?1:1-powf(2,-10*t); }
static inline float ease_in_circ(float t)     { return 1-sqrtf(fmaxf(0,1-t*t)); }
static inline float ease_out_circ(float t)    { float s=t-1; return sqrtf(fmaxf(0,1-s*s)); }
static inline float ease_in_elastic(float t)  { if(t==0||t==1)return t; return -powf(2,10*(t-1))*sinf((t-1.1f)*5*MATH_PI); }
static inline float ease_out_elastic(float t) { if(t==0||t==1)return t; return powf(2,-10*t)*sinf((t-.1f)*5*MATH_PI)+1; }
static inline float ease_out_bounce(float t)  { if(t<1/2.75f)return 7.5625f*t*t; if(t<2/2.75f){t-=1.5f/2.75f;return 7.5625f*t*t+.75f;} if(t<2.5f/2.75f){t-=2.25f/2.75f;return 7.5625f*t*t+.9375f;} t-=2.625f/2.75f;return 7.5625f*t*t+.984375f; }
static inline float ease_in_bounce(float t)   { return 1-ease_out_bounce(1-t); }
static inline float ease_out_back(float t)    { float c=1.70158f,s=t-1; return s*s*((c+1)*s+c)+1; }
static inline float ease_in_back(float t)     { float c=1.70158f; return t*t*((c+1)*t-c); }
static inline float ease_inout_back(float t)  { float c=1.70158f*1.525f; if(t<.5f)return 2*t*t*((c+1)*2*t-c); float s=2*t-2; return .5f*(s*s*((c+1)*s+c)+2); }

/* ================================================================
   SCALAR MATH UTILITIES
================================================================ */
static inline float lerp(float a,float b,float t)         { return a+(b-a)*t; }
static inline float clamp(float v,float mn,float mx)      { return v<mn?mn:v>mx?mx:v; }
static inline float clamp01(float v)                       { return v<0?0:v>1?1:v; }
static inline float smoothstep(float e0,float e1,float x) { float t=clamp((x-e0)/(e1-e0),0,1); return t*t*(3-2*t); }
static inline float smootherstep(float e0,float e1,float x){ float t=clamp((x-e0)/(e1-e0),0,1); return t*t*t*(t*(t*6-15)+10); }
static inline float remap(float v,float a0,float a1,float b0,float b1){ return b0+(v-a0)/(a1-a0)*(b1-b0); }
static inline float remap_clamp(float v,float a0,float a1,float b0,float b1){ return lerp(b0,b1,clamp01((v-a0)/(a1-a0))); }
static inline float wrap(float v,float lo,float hi)       { float r=hi-lo; return v-(floorf((v-lo)/r)*r); }
static inline float ping_pong(float t,float length)       { t=wrap(t,0,length*2); return length-fabsf(t-length); }
static inline float snap(float v,float step)              { return floorf(v/step+.5f)*step; }
static inline float sign_f(float x)                       { return (float)((x>0)-(x<0)); }
static inline int   sign_i(int x)                         { return (x>0)-(x<0); }
static inline float move_towards_f(float cur,float tgt,float step){ float d=tgt-cur; if(fabsf(d)<=step)return tgt; return cur+sign_f(d)*step; }
static inline float delta_angle(float a,float b)          { float d=wrap(b-a,-180,180); return d; }
static inline float move_towards_angle(float cur,float tgt,float step){ return cur+move_towards_f(0,delta_angle(cur,tgt),step); }

/* Fast approximations */
static inline float fast_rsqrt(float n){ int i; float x2=n*.5f,y=n; memcpy(&i,&y,4); i=0x5f3759df-(i>>1); memcpy(&y,&i,4); return y*(1.5f-x2*y*y); }
static inline float fast_sin(float x)  { x=x-floorf(x/(MATH_TAU))*MATH_TAU; if(x>MATH_PI)x-=MATH_TAU; float s=x,x2=x*x; s+=x2*x*(-0.16666667f+x2*(0.00833333f-x2*0.000198413f)); return s; }
static inline float fast_cos(float x)  { return fast_sin(x+MATH_PI_HALF); }

/* ================================================================
   HSV <-> RGB
================================================================ */
typedef struct { float h,s,v; } HSV; /* h: 0-360, s/v: 0-1 */
typedef struct { float r,g,b; } RGB; /* 0-1 */

static inline RGB hsv_to_rgb(HSV c){
    float h=c.h/60.f,s=c.s,v=c.v;
    int i=(int)h; float f=h-i;
    float p=v*(1-s),q=v*(1-s*f),t=v*(1-s*(1-f));
    switch(i%6){ case 0:return (RGB){v,t,p}; case 1:return (RGB){q,v,p}; case 2:return (RGB){p,v,t}; case 3:return (RGB){p,q,v}; case 4:return (RGB){t,p,v}; default:return (RGB){v,p,q}; }
}
static inline HSV rgb_to_hsv(RGB c){
    float mx=fmaxf(c.r,fmaxf(c.g,c.b)),mn=fminf(c.r,fminf(c.g,c.b)),d=mx-mn;
    float h=0,s=(mx==0)?0:d/mx;
    if(d>0){ if(mx==c.r)h=60*(fmodf((c.g-c.b)/d,6)); else if(mx==c.g)h=60*((c.b-c.r)/d+2); else h=60*((c.r-c.g)/d+4); if(h<0)h+=360; }
    return (HSV){h,s,mx};
}

/* ================================================================
   RNG (xorshift64)
================================================================ */
typedef struct { uint64_t state; } Rng;
static inline void    rng_seed(Rng* r,uint64_t s)    { r->state=s?s:0x123456789ABCDEF0ULL; }
static inline uint64_t rng_u64(Rng* r)               { r->state^=r->state<<13; r->state^=r->state>>7; r->state^=r->state<<17; return r->state; }
static inline uint32_t rng_u32(Rng* r)               { return (uint32_t)(rng_u64(r)>>32); }
static inline float    rng_float(Rng* r)              { return (float)(rng_u64(r)>>40)/(float)(1<<24); }
static inline float    rng_range(Rng* r,float lo,float hi){ return lo+(hi-lo)*rng_float(r); }
static inline int      rng_int(Rng* r,int lo,int hi)  { return lo+(int)(rng_u32(r)%(unsigned)(hi-lo)); }
static inline Vec2     rng_vec2(Rng* r,float lo,float hi){ return vec2(rng_range(r,lo,hi),rng_range(r,lo,hi)); }
static inline Vec2     rng_circle(Rng* r)             { float a=rng_float(r)*MATH_TAU,rd=sqrtf(rng_float(r)); return vec2(rd*cosf(a),rd*sinf(a)); }
static inline Vec2     rng_direction(Rng* r)          { float a=rng_float(r)*MATH_TAU; return vec2(cosf(a),sinf(a)); }
static inline float    rng_normal(Rng* r,float mean,float sd){ float u=1.f-rng_float(r),v=rng_float(r); return mean+sd*sqrtf(-2.f*logf(u))*cosf(MATH_TAU*v); }

/* Глобальный RNG (обратная совместимость) */
float random_float(void);
float random_range(float min,float max);
int   random_int(int lo,int hi);
Vec2  random_vec2(float min_x,float max_x,float min_y,float max_y);

/* ================================================================
   ШУМ
================================================================ */
float noise1d(float x,float t);
float noise2d(float x,float y);
float fbm(float x,float y,int octaves);
float perlin(float x,float y);
float worley(float x,float y);   /* клеточный / Voronoi-like */
float simplex2(float x,float y); /* Simplex 2D */

#endif /* ENGINE_MATH_H */
