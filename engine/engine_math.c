/* engine_math.c — Cngine v2
 * LGPL v3 | Chikatilo / sooborn / SGLauncher */
#include "engine_math.h"
#include <stdio.h>
#include <time.h>

/* ================================================================
   ГЛОБАЛЬНЫЙ RNG (xorshift)
================================================================ */
static uint64_t g_rng_state = 0xDEADBEEFCAFEBABEULL;
static uint64_t global_rng_next(void){
    g_rng_state^=g_rng_state<<13;
    g_rng_state^=g_rng_state>>7;
    g_rng_state^=g_rng_state<<17;
    return g_rng_state;
}
float random_float(void){ return (float)(global_rng_next()>>40)/(float)(1<<24); }
float random_range(float mn,float mx){ return mn+(mx-mn)*random_float(); }
int   random_int(int lo,int hi){ return lo+(int)(((uint32_t)global_rng_next())%(unsigned)(hi-lo)); }
Vec2  random_vec2(float mnx,float mxx,float mny,float mxy){ return vec2(random_range(mnx,mxx),random_range(mny,mxy)); }

/* ================================================================
   МАТРИЦЫ 2x2
================================================================ */
Mat2 mat2_identity(void){ Mat2 m={{{1,0},{0,1}}}; return m; }
Mat2 mat2_rotation(float a){ float ca=cosf(a),sa=sinf(a); Mat2 m={{{ca,-sa},{sa,ca}}}; return m; }
Mat2 mat2_scale(float sx,float sy){ Mat2 m={{{sx,0},{0,sy}}}; return m; }
Vec2 mat2_mul_vec2(Mat2 m,Vec2 v){ return vec2(m.m[0][0]*v.x+m.m[0][1]*v.y, m.m[1][0]*v.x+m.m[1][1]*v.y); }

/* ================================================================
   МАТРИЦЫ 3x3
================================================================ */
Mat3 mat3_identity(void){ Mat3 m={{{1,0,0},{0,1,0},{0,0,1}}}; return m; }
Mat3 mat3_translation(Vec2 t){ Mat3 m=mat3_identity(); m.m[0][2]=t.x; m.m[1][2]=t.y; return m; }
Mat3 mat3_rotation(float a){ float ca=cosf(a),sa=sinf(a); Mat3 m={{{ca,-sa,0},{sa,ca,0},{0,0,1}}}; return m; }
Mat3 mat3_scale(float sx,float sy){ Mat3 m={{{sx,0,0},{0,sy,0},{0,0,1}}}; return m; }
Mat3 mat3_mul(Mat3 a,Mat3 b){
    Mat3 r;
    for(int i=0;i<3;i++) for(int j=0;j<3;j++){
        r.m[i][j]=0;
        for(int k=0;k<3;k++) r.m[i][j]+=a.m[i][k]*b.m[k][j];
    }
    return r;
}
Vec2 mat3_mul_vec2(Mat3 m,Vec2 v){ return vec2(m.m[0][0]*v.x+m.m[0][1]*v.y+m.m[0][2], m.m[1][0]*v.x+m.m[1][1]*v.y+m.m[1][2]); }
Mat3 mat3_transpose(Mat3 m){ Mat3 r; for(int i=0;i<3;i++) for(int j=0;j<3;j++) r.m[i][j]=m.m[j][i]; return r; }
Mat3 mat3_inverse(Mat3 m){
    float d = m.m[0][0]*(m.m[1][1]*m.m[2][2]-m.m[1][2]*m.m[2][1])
             -m.m[0][1]*(m.m[1][0]*m.m[2][2]-m.m[1][2]*m.m[2][0])
             +m.m[0][2]*(m.m[1][0]*m.m[2][1]-m.m[1][1]*m.m[2][0]);
    if(fabsf(d)<MATH_EPSILON)return mat3_identity();
    float id=1.f/d;
    Mat3 r;
    r.m[0][0]= (m.m[1][1]*m.m[2][2]-m.m[1][2]*m.m[2][1])*id;
    r.m[0][1]=-(m.m[0][1]*m.m[2][2]-m.m[0][2]*m.m[2][1])*id;
    r.m[0][2]= (m.m[0][1]*m.m[1][2]-m.m[0][2]*m.m[1][1])*id;
    r.m[1][0]=-(m.m[1][0]*m.m[2][2]-m.m[1][2]*m.m[2][0])*id;
    r.m[1][1]= (m.m[0][0]*m.m[2][2]-m.m[0][2]*m.m[2][0])*id;
    r.m[1][2]=-(m.m[0][0]*m.m[1][2]-m.m[0][2]*m.m[1][0])*id;
    r.m[2][0]= (m.m[1][0]*m.m[2][1]-m.m[1][1]*m.m[2][0])*id;
    r.m[2][1]=-(m.m[0][0]*m.m[2][1]-m.m[0][1]*m.m[2][0])*id;
    r.m[2][2]= (m.m[0][0]*m.m[1][1]-m.m[0][1]*m.m[1][0])*id;
    return r;
}

/* ================================================================
   МАТРИЦЫ 4x4
================================================================ */
Mat4 mat4_identity(void){
    Mat4 m={{{0}}};
    m.m[0][0]=m.m[1][1]=m.m[2][2]=m.m[3][3]=1;
    return m;
}
Mat4 mat4_translation(Vec3 t){
    Mat4 m=mat4_identity();
    m.m[0][3]=t.x; m.m[1][3]=t.y; m.m[2][3]=t.z;
    return m;
}
Mat4 mat4_rotation_x(float a){
    Mat4 m=mat4_identity();
    float ca=cosf(a),sa=sinf(a);
    m.m[1][1]=ca; m.m[1][2]=-sa;
    m.m[2][1]=sa; m.m[2][2]=ca;
    return m;
}
Mat4 mat4_rotation_y(float a){
    Mat4 m=mat4_identity();
    float ca=cosf(a),sa=sinf(a);
    m.m[0][0]=ca; m.m[0][2]=sa;
    m.m[2][0]=-sa;m.m[2][2]=ca;
    return m;
}
Mat4 mat4_rotation_z(float a){
    Mat4 m=mat4_identity();
    float ca=cosf(a),sa=sinf(a);
    m.m[0][0]=ca; m.m[0][1]=-sa;
    m.m[1][0]=sa; m.m[1][1]=ca;
    return m;
}
Mat4 mat4_rotation_quat(Quat q){
    Quat nq=quat_normalize(q);
    float x=nq.x,y=nq.y,z=nq.z,w=nq.w;
    Mat4 m=mat4_identity();
    m.m[0][0]=1-2*(y*y+z*z); m.m[0][1]=2*(x*y-z*w);   m.m[0][2]=2*(x*z+y*w);
    m.m[1][0]=2*(x*y+z*w);   m.m[1][1]=1-2*(x*x+z*z); m.m[1][2]=2*(y*z-x*w);
    m.m[2][0]=2*(x*z-y*w);   m.m[2][1]=2*(y*z+x*w);   m.m[2][2]=1-2*(x*x+y*y);
    return m;
}
Mat4 mat4_scale3(Vec3 s){
    Mat4 m=mat4_identity();
    m.m[0][0]=s.x; m.m[1][1]=s.y; m.m[2][2]=s.z;
    return m;
}
Mat4 mat4_mul(Mat4 a,Mat4 b){
    Mat4 r={{{0}}};
    for(int i=0;i<4;i++) for(int j=0;j<4;j++) for(int k=0;k<4;k++) r.m[i][j]+=a.m[i][k]*b.m[k][j];
    return r;
}
Vec4 mat4_mul_vec4(Mat4 m,Vec4 v){
    return vec4(
        m.m[0][0]*v.x+m.m[0][1]*v.y+m.m[0][2]*v.z+m.m[0][3]*v.w,
        m.m[1][0]*v.x+m.m[1][1]*v.y+m.m[1][2]*v.z+m.m[1][3]*v.w,
        m.m[2][0]*v.x+m.m[2][1]*v.y+m.m[2][2]*v.z+m.m[2][3]*v.w,
        m.m[3][0]*v.x+m.m[3][1]*v.y+m.m[3][2]*v.z+m.m[3][3]*v.w);
}
Vec3 mat4_mul_point(Mat4 m,Vec3 v){ Vec4 r=mat4_mul_vec4(m,vec4(v.x,v.y,v.z,1)); return vec3(r.x,r.y,r.z); }
Vec3 mat4_mul_dir(Mat4 m,Vec3 v)  { Vec4 r=mat4_mul_vec4(m,vec4(v.x,v.y,v.z,0)); return vec3(r.x,r.y,r.z); }

Mat4 mat4_trs(Vec3 t,Quat r_q,Vec3 s){
    Mat4 T=mat4_translation(t);
    Mat4 R=mat4_rotation_quat(r_q);
    Mat4 S=mat4_scale3(s);
    return mat4_mul(T,mat4_mul(R,S));
}
Mat4 mat4_transpose(Mat4 m){ Mat4 r={{{0}}}; for(int i=0;i<4;i++) for(int j=0;j<4;j++) r.m[i][j]=m.m[j][i]; return r; }

/* Обратная матрица 4x4 (аффинный случай — быстро) */
Mat4 mat4_inverse(Mat4 m){
    /* Полный Крамер для 4x4 */
    float s0=m.m[0][0]*m.m[1][1]-m.m[1][0]*m.m[0][1];
    float s1=m.m[0][0]*m.m[1][2]-m.m[1][0]*m.m[0][2];
    float s2=m.m[0][0]*m.m[1][3]-m.m[1][0]*m.m[0][3];
    float s3=m.m[0][1]*m.m[1][2]-m.m[1][1]*m.m[0][2];
    float s4=m.m[0][1]*m.m[1][3]-m.m[1][1]*m.m[0][3];
    float s5=m.m[0][2]*m.m[1][3]-m.m[1][2]*m.m[0][3];
    float c5=m.m[2][2]*m.m[3][3]-m.m[3][2]*m.m[2][3];
    float c4=m.m[2][1]*m.m[3][3]-m.m[3][1]*m.m[2][3];
    float c3=m.m[2][1]*m.m[3][2]-m.m[3][1]*m.m[2][2];
    float c2=m.m[2][0]*m.m[3][3]-m.m[3][0]*m.m[2][3];
    float c1=m.m[2][0]*m.m[3][2]-m.m[3][0]*m.m[2][2];
    float c0=m.m[2][0]*m.m[3][1]-m.m[3][0]*m.m[2][1];
    float det=s0*c5-s1*c4+s2*c3+s3*c2-s4*c1+s5*c0;
    if(fabsf(det)<MATH_EPSILON)return mat4_identity();
    float id=1.f/det;
    Mat4 r={{{0}}};
    r.m[0][0]=( m.m[1][1]*c5-m.m[1][2]*c4+m.m[1][3]*c3)*id;
    r.m[0][1]=(-m.m[0][1]*c5+m.m[0][2]*c4-m.m[0][3]*c3)*id;
    r.m[0][2]=( m.m[3][1]*s5-m.m[3][2]*s4+m.m[3][3]*s3)*id;
    r.m[0][3]=(-m.m[2][1]*s5+m.m[2][2]*s4-m.m[2][3]*s3)*id;
    r.m[1][0]=(-m.m[1][0]*c5+m.m[1][2]*c2-m.m[1][3]*c1)*id;
    r.m[1][1]=( m.m[0][0]*c5-m.m[0][2]*c2+m.m[0][3]*c1)*id;
    r.m[1][2]=(-m.m[3][0]*s5+m.m[3][2]*s2-m.m[3][3]*s1)*id;
    r.m[1][3]=( m.m[2][0]*s5-m.m[2][2]*s2+m.m[2][3]*s1)*id;
    r.m[2][0]=( m.m[1][0]*c4-m.m[1][1]*c2+m.m[1][3]*c0)*id;
    r.m[2][1]=(-m.m[0][0]*c4+m.m[0][1]*c2-m.m[0][3]*c0)*id;
    r.m[2][2]=( m.m[3][0]*s4-m.m[3][1]*s2+m.m[3][3]*s0)*id;
    r.m[2][3]=(-m.m[2][0]*s4+m.m[2][1]*s2-m.m[2][3]*s0)*id;
    r.m[3][0]=(-m.m[1][0]*c3+m.m[1][1]*c1-m.m[1][2]*c0)*id;
    r.m[3][1]=( m.m[0][0]*c3-m.m[0][1]*c1+m.m[0][2]*c0)*id;
    r.m[3][2]=(-m.m[3][0]*s3+m.m[3][1]*s1-m.m[3][2]*s0)*id;
    r.m[3][3]=( m.m[2][0]*s3-m.m[2][1]*s1+m.m[2][2]*s0)*id;
    return r;
}

Mat4 mat4_ortho(float l,float r2,float b,float top,float near,float far){
    Mat4 m=mat4_identity();
    m.m[0][0]=2/(r2-l);      m.m[0][3]=-(r2+l)/(r2-l);
    m.m[1][1]=2/(top-b);     m.m[1][3]=-(top+b)/(top-b);
    m.m[2][2]=-2/(far-near); m.m[2][3]=-(far+near)/(far-near);
    return m;
}
Mat4 mat4_perspective(float fov_y,float aspect,float near,float far){
    Mat4 m={{{0}}};
    float th=tanf(fov_y*.5f);
    m.m[0][0]=1.f/(aspect*th);
    m.m[1][1]=1.f/th;
    m.m[2][2]=-(far+near)/(far-near);
    m.m[2][3]=-2*far*near/(far-near);
    m.m[3][2]=-1;
    return m;
}
Mat4 mat4_look_at(Vec3 eye,Vec3 target,Vec3 up){
    Vec3 fwd=vec3_normalize(vec3_sub(target,eye));
    Vec3 right=vec3_normalize(vec3_cross(fwd,up));
    Vec3 u=vec3_cross(right,fwd);
    Mat4 m=mat4_identity();
    m.m[0][0]=right.x; m.m[0][1]=right.y; m.m[0][2]=right.z; m.m[0][3]=-vec3_dot(right,eye);
    m.m[1][0]=u.x;     m.m[1][1]=u.y;     m.m[1][2]=u.z;     m.m[1][3]=-vec3_dot(u,eye);
    m.m[2][0]=-fwd.x;  m.m[2][1]=-fwd.y;  m.m[2][2]=-fwd.z;  m.m[2][3]= vec3_dot(fwd,eye);
    return m;
}

/* ================================================================
   КВАТЕРНИОН — не-inline функции
================================================================ */
Quat quat_from_axis_angle(Vec3 axis,float angle){
    Vec3 n=vec3_normalize(axis);
    float s=sinf(angle*.5f),c=cosf(angle*.5f);
    return quat(n.x*s,n.y*s,n.z*s,c);
}
Quat quat_from_euler(float pitch,float yaw,float roll){
    float cy=cosf(yaw*.5f),sy=sinf(yaw*.5f);
    float cp=cosf(pitch*.5f),sp=sinf(pitch*.5f);
    float cr=cosf(roll*.5f),sr=sinf(roll*.5f);
    return quat(sr*cp*cy-cr*sp*sy,cr*sp*cy+sr*cp*sy,cr*cp*sy-sr*sp*cy,cr*cp*cy+sr*sp*sy);
}
Quat quat_slerp(Quat a,Quat b,float t){
    float d=a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w;
    if(d<0){b=quat(-b.x,-b.y,-b.z,-b.w);d=-d;}
    if(d>0.9995f){ Quat r=quat(a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t,a.w+(b.w-a.w)*t); return quat_normalize(r); }
    float th=acosf(d),sth=sinf(th);
    return quat(a.x*sinf((1-t)*th)/sth+b.x*sinf(t*th)/sth,
                a.y*sinf((1-t)*th)/sth+b.y*sinf(t*th)/sth,
                a.z*sinf((1-t)*th)/sth+b.z*sinf(t*th)/sth,
                a.w*sinf((1-t)*th)/sth+b.w*sinf(t*th)/sth);
}
Vec3 quat_to_euler(Quat q){
    float sinr_cosp=2*(q.w*q.x+q.y*q.z);
    float cosr_cosp=1-2*(q.x*q.x+q.y*q.y);
    float roll=atan2f(sinr_cosp,cosr_cosp);
    float sinp=2*(q.w*q.y-q.z*q.x);
    float pitch=(fabsf(sinp)>=1)?copysignf(MATH_PI_HALF,sinp):asinf(sinp);
    float siny_cosp=2*(q.w*q.z+q.x*q.y);
    float cosy_cosp=1-2*(q.y*q.y+q.z*q.z);
    float yaw=atan2f(siny_cosp,cosy_cosp);
    return vec3(pitch,yaw,roll);
}

/* ================================================================
   ГЕОМЕТРИЯ
================================================================ */
bool rect_contains_point(Rect r,Vec2 p){
    return p.x>=r.x&&p.x<=r.x+r.w&&p.y>=r.y&&p.y<=r.y+r.h;
}
bool circle_contains_point(Circle c,Vec2 p){
    return vec2_distance_sq(c.center,p)<=c.radius*c.radius;
}
bool line_intersect_line(Line a,Line b,Vec2* out){
    Vec2 r=vec2_sub(a.end,a.start), s=vec2_sub(b.end,b.start);
    float rxs=vec2_cross(r,s);
    if(fabsf(rxs)<MATH_EPSILON)return false;
    Vec2 c=vec2_sub(b.start,a.start);
    float t=vec2_cross(c,s)/rxs, u=vec2_cross(c,r)/rxs;
    if(t>=0&&t<=1&&u>=0&&u<=1){
        if(out)*out=vec2_add(a.start,vec2_mul(r,t));
        return true;
    }
    return false;
}
bool line_intersect_circle(Line l,Circle c,Vec2* out){
    Vec2 d=vec2_sub(l.end,l.start);
    Vec2 f=vec2_sub(l.start,c.center);
    float a=vec2_dot(d,d);
    float b=2*vec2_dot(f,d);
    float k=vec2_dot(f,f)-c.radius*c.radius;
    float disc=b*b-4*a*k;
    if(disc<0)return false;
    float sq=sqrtf(disc);
    float t1=(-b-sq)/(2*a), t2=(-b+sq)/(2*a);
    float t=(t1>=0&&t1<=1)?t1:((t2>=0&&t2<=1)?t2:-1);
    if(t<0)return false;
    if(out)*out=vec2_add(l.start,vec2_mul(d,t));
    return true;
}
bool circle_intersect_circle(Circle a,Circle b,Vec2* pen){
    Vec2 d=vec2_sub(b.center,a.center);
    float d2=vec2_length_sq(d);
    float r=a.radius+b.radius;
    if(d2>=r*r)return false;
    if(pen){
        float dist=sqrtf(d2);
        float overlap=r-dist;
        Vec2 n=dist>MATH_EPSILON?vec2_mul(d,1/dist):vec2(1,0);
        *pen=vec2_mul(n,overlap);
    }
    return true;
}

/* OBB */
bool obb_contains_point(OBB o,Vec2 p){
    Vec2 d=vec2_sub(p,o.center);
    Vec2 ax=vec2_from_angle(o.angle),ay=vec2_perp(ax);
    float dx=fabsf(vec2_dot(d,ax)), dy=fabsf(vec2_dot(d,ay));
    return dx<=o.half_extents.x&&dy<=o.half_extents.y;
}
/* SAT для двух OBB */
bool obb_overlap_obb(OBB a,OBB b,Vec2* mtv){
    Vec2 axes[4];
    axes[0]=vec2_from_angle(a.angle);
    axes[1]=vec2_perp(axes[0]);
    axes[2]=vec2_from_angle(b.angle);
    axes[3]=vec2_perp(axes[2]);

    float min_overlap=MATH_INF;
    Vec2  min_axis=vec2_zero();

    Vec2 corners_a[4], corners_b[4];
    Vec2 ax_a=axes[0],ay_a=axes[1];
    Vec2 ax_b=axes[2],ay_b=axes[3];
    corners_a[0]=vec2_add(a.center,vec2_add(vec2_mul(ax_a,-a.half_extents.x),vec2_mul(ay_a,-a.half_extents.y)));
    corners_a[1]=vec2_add(a.center,vec2_add(vec2_mul(ax_a, a.half_extents.x),vec2_mul(ay_a,-a.half_extents.y)));
    corners_a[2]=vec2_add(a.center,vec2_add(vec2_mul(ax_a, a.half_extents.x),vec2_mul(ay_a, a.half_extents.y)));
    corners_a[3]=vec2_add(a.center,vec2_add(vec2_mul(ax_a,-a.half_extents.x),vec2_mul(ay_a, a.half_extents.y)));
    corners_b[0]=vec2_add(b.center,vec2_add(vec2_mul(ax_b,-b.half_extents.x),vec2_mul(ay_b,-b.half_extents.y)));
    corners_b[1]=vec2_add(b.center,vec2_add(vec2_mul(ax_b, b.half_extents.x),vec2_mul(ay_b,-b.half_extents.y)));
    corners_b[2]=vec2_add(b.center,vec2_add(vec2_mul(ax_b, b.half_extents.x),vec2_mul(ay_b, b.half_extents.y)));
    corners_b[3]=vec2_add(b.center,vec2_add(vec2_mul(ax_b,-b.half_extents.x),vec2_mul(ay_b, b.half_extents.y)));

    for(int i=0;i<4;i++){
        Vec2 axis=axes[i];
        float mn_a=MATH_INF, mx_a=-MATH_INF, mn_b=MATH_INF, mx_b=-MATH_INF;
        for(int j=0;j<4;j++){
            float pa=vec2_dot(corners_a[j],axis);
            float pb=vec2_dot(corners_b[j],axis);
            if(pa<mn_a)mn_a=pa; if(pa>mx_a)mx_a=pa;
            if(pb<mn_b)mn_b=pb; if(pb>mx_b)mx_b=pb;
        }
        float overlap=fminf(mx_a,mx_b)-fmaxf(mn_a,mn_b);
        if(overlap<0)return false;
        if(overlap<min_overlap){ min_overlap=overlap; min_axis=axis; }
    }
    if(mtv){
        Vec2 d=vec2_sub(b.center,a.center);
        if(vec2_dot(d,min_axis)<0)min_axis=vec2_neg(min_axis);
        *mtv=vec2_mul(min_axis,min_overlap);
    }
    return true;
}

/* ================================================================
   ШУМ
================================================================ */
/* Базовый value noise */
static float _hash2(int ix,int iy){
    unsigned h=(unsigned)(ix*1619+iy*31337)*0x45d9f3b; h^=h>>16; return (h&0xFF)/255.f;
}
static float _hash1(int ix,int it){
    unsigned h=(unsigned)(ix*1619+it*31337)*0x45d9f3b; h^=h>>16; return (h&0xFF)/255.f;
}

float noise1d(float x,float t){
    int ix=(int)floorf(x); float fx=x-ix;
    fx=fx*fx*(3-2*fx);
    int it=(int)(t*80);
    float v0=_hash1(ix,it), v1=_hash1(ix+1,it);
    return v0+(v1-v0)*fx;
}
float noise2d(float x,float y){
    int ix=(int)floorf(x),iy=(int)floorf(y);
    float fx=x-ix,fy=y-iy;
    fx=fx*fx*(3-2*fx); fy=fy*fy*(3-2*fy);
    float v00=_hash2(ix,iy),v10=_hash2(ix+1,iy),v01=_hash2(ix,iy+1),v11=_hash2(ix+1,iy+1);
    return v00+(v10-v00)*fx+(v01-v00)*fy+(v00-v10-v01+v11)*fx*fy;
}
float fbm(float x,float y,int octaves){
    float val=0,amp=0.5f,freq=1,mx=0;
    for(int i=0;i<octaves;i++){ val+=noise2d(x*freq,y*freq)*amp; mx+=amp; amp*=0.5f; freq*=2; }
    return val/mx;
}
float perlin(float x,float y){
    int ix=(int)floorf(x)&255,iy=(int)floorf(y)&255;
    float fx=x-floorf(x),fy=y-floorf(y);
    float u=fx*fx*fx*(fx*(fx*6-15)+10), v=fy*fy*fy*(fy*(fy*6-15)+10);
    /* Градиенты через хеш */
    unsigned ha=(unsigned)(ix*1619+iy*31337)*0x45d9f3b;    ha^=ha>>16;
    unsigned hb=(unsigned)((ix+1)*1619+iy*31337)*0x45d9f3b; hb^=hb>>16;
    unsigned hc=(unsigned)(ix*1619+(iy+1)*31337)*0x45d9f3b; hc^=hc>>16;
    unsigned hd=(unsigned)((ix+1)*1619+(iy+1)*31337)*0x45d9f3b; hd^=hd>>16;
    /* Проекция градиента */
    static const float gx[4]={1,-1,0,0},gy[4]={0,0,1,-1};
    int ga=ha&3,gb=hb&3,gc=hc&3,gd2=hd&3;
    float n00=gx[ga]*fx+gy[ga]*fy;
    float n10=gx[gb]*(fx-1)+gy[gb]*fy;
    float n01=gx[gc]*fx+gy[gc]*(fy-1);
    float n11=gx[gd2]*(fx-1)+gy[gd2]*(fy-1);
    float nx0=n00+(n10-n00)*u;
    float nx1=n01+(n11-n01)*u;
    return (nx0+(nx1-nx0)*v)*0.5f+0.5f;
}

/* Клеточный/Worley шум (F1 — расстояние до ближайшей точки) */
float worley(float x,float y){
    int ix=(int)floorf(x),iy=(int)floorf(y);
    float min_d=MATH_INF;
    for(int dy=-1;dy<=1;dy++) for(int dx=-1;dx<=1;dx++){
        int cx=ix+dx,cy=iy+dy;
        unsigned hx=(unsigned)(cx*1619+cy*31337)*0x45d9f3b; hx^=hx>>16;
        unsigned hy=(unsigned)(cx*31337+cy*1619)*0x45d9f3b; hy^=hy>>16;
        float px=(float)cx+(hx&0xFFFF)/65535.f;
        float py=(float)cy+(hy&0xFFFF)/65535.f;
        float d=(x-px)*(x-px)+(y-py)*(y-py);
        if(d<min_d)min_d=d;
    }
    return sqrtf(min_d);
}

/* Simplex 2D */
static float _sdot(int gx,int gy,float x,float y){ return gx*x+gy*y; }
static const int _simplex_grad[][2]={{1,1},{-1,1},{1,-1},{-1,-1},{1,0},{-1,0},{0,1},{0,-1}};
static int _perm[512];
static int _simplex_init=0;
static void _simplex_setup(void){
    if(_simplex_init)return;
    for(int i=0;i<256;i++) _perm[i]=i;
    /* Fischer-Yates shuffle с детерминированным seed */
    uint64_t st=0xDEADBEEF01234567ULL;
    for(int i=255;i>0;i--){
        st^=st<<13;st^=st>>7;st^=st<<17;
        int j=(int)(st%(unsigned)(i+1));
        int tmp=_perm[i];_perm[i]=_perm[j];_perm[j]=tmp;
    }
    for(int i=0;i<256;i++)_perm[256+i]=_perm[i];
    _simplex_init=1;
}
float simplex2(float x,float y){
    _simplex_setup();
    const float F2=0.3660254f,G2=0.2113249f;
    float s=(x+y)*F2;
    int i=(int)floorf(x+s),j=(int)floorf(y+s);
    float t=(i+j)*G2;
    float x0=x-(i-t),y0=y-(j-t);
    int i1=x0>y0?1:0,j1=x0>y0?0:1;
    float x1=x0-i1+G2,y1=y0-j1+G2;
    float x2=x0-1+2*G2,y2=y0-1+2*G2;
    int ii=i&255,jj=j&255;
    int gi0=_perm[ii+_perm[jj]]&7;
    int gi1=_perm[ii+i1+_perm[jj+j1]]&7;
    int gi2=_perm[ii+1+_perm[jj+1]]&7;
    float n0=0,n1=0,n2=0;
    float t0=.5f-x0*x0-y0*y0; if(t0>=0){t0*=t0;n0=t0*t0*_sdot(_simplex_grad[gi0][0],_simplex_grad[gi0][1],x0,y0);}
    float t1=.5f-x1*x1-y1*y1; if(t1>=0){t1*=t1;n1=t1*t1*_sdot(_simplex_grad[gi1][0],_simplex_grad[gi1][1],x1,y1);}
    float t2=.5f-x2*x2-y2*y2; if(t2>=0){t2*=t2;n2=t2*t2*_sdot(_simplex_grad[gi2][0],_simplex_grad[gi2][1],x2,y2);}
    return .5f+35.f*(n0+n1+n2);
}
