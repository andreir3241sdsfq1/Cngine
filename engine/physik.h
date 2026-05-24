/* physik.h — Cngine v2
 * LGPL v3 | Chikatilo / sooborn / SGLauncher
 *
 * НОВОЕ v2:
 *   RigidBody (масса, инерция, угловая скорость)
 *   DistanceJoint, RevoluteJoint (шарниры)
 *   Raycast 2D (против частиц и AABB)
 *   TriggerZone (enter/exit callbacks)
 *   ForceField (аттрактор, репеллер, вихрь, turbulence)
 *   BreakableSpring (разрывается при превышении силы)
 *   Улучшенный Spatial Hash с динамическим размером ячейки
 *   Verlet интеграция (опционально)
 *   Constraint solver итерации (PBD)
 */
#ifndef PHYSIK_H
#define PHYSIK_H

#include "engine_math.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

/* ================================================================
   ТИПЫ ЧАСТИЦ
================================================================ */
typedef enum {
    PARTICLE_TYPE_NORMAL,
    PARTICLE_TYPE_STATIC,
    PARTICLE_TYPE_WATER,
    PARTICLE_TYPE_SAND,
    PARTICLE_TYPE_GAS,
    PARTICLE_TYPE_CLOTH,
    PARTICLE_TYPE_SOFT_BODY,
    PARTICLE_TYPE_RIGID,
    PARTICLE_TYPE_PROJECTILE,
    PARTICLE_TYPE_TRIGGER,    /* проходит сквозь, но детектирует */
    PARTICLE_TYPE_COUNT
} ParticleType;

/* Слои коллизий (bitmask) */
#define LAYER_DEFAULT   (1<<0)
#define LAYER_PLAYER    (1<<1)
#define LAYER_ENEMY     (1<<2)
#define LAYER_PROJECTILE (1<<3)
#define LAYER_TRIGGER   (1<<4)
#define LAYER_TERRAIN   (1<<5)
#define LAYER_ALL       0xFFFF

/* ================================================================
   ЧАСТИЦА (Particle)
================================================================ */
typedef struct Particle {
    Vec2 position;
    Vec2 velocity;
    Vec2 force;
    Vec2 prev_position;  /* для Verlet */

    float mass;
    float inv_mass;
    float radius;

    ParticleType type;
    bool  active;
    float lifetime;      /* -1 = бесконечно */
    float friction;
    float restitution;
    float damping;       /* дополнительное затухание на частицу */
    float angular_velocity;
    float angle;

    uint16_t layer;      /* слой коллизии */
    uint16_t mask;       /* маска — с кем сталкивается */

    void* user_data;

    /* Соседи (мягкие тела) */
    struct Particle** neighbors;
    int    neighbor_count;
    float* rest_distances;

    /* Цвет (для дебага) */
    uint32_t debug_color; /* 0xRRGGBBAA */
} Particle;

/* ================================================================
   ПРУЖИНА (Spring)
================================================================ */
typedef struct {
    Particle* a;
    Particle* b;
    float rest_distance;
    float stiffness;
    float damping;
    float break_force;   /* 0 = нерушима */
    bool  active;
    bool  is_structural;
    bool  is_shear;
    bool  is_bend;
} Spring;

/* ================================================================
   JOINT — ДИСТАНЦИЯ
================================================================ */
typedef struct {
    Particle* a;
    Particle* b;
    float min_distance;  /* 0 = только ограничение сверху */
    float max_distance;
    float stiffness;
    bool  active;
} DistanceJoint;

/* ================================================================
   JOINT — REVOLUTE (шарнир)
================================================================ */
typedef struct {
    Particle* anchor;    /* неподвижная точка или NULL */
    Vec2      pivot;
    Particle* body;
    float     angle_min; /* радианы, 0 = нет ограничения */
    float     angle_max;
    float     motor_speed;
    float     motor_torque;
    bool      active;
} RevoluteJoint;

/* ================================================================
   RAYCAST
================================================================ */
typedef struct {
    bool    hit;
    Vec2    point;
    Vec2    normal;
    float   distance;
    Particle* particle;  /* NULL если не частица */
} RaycastHit;

/* ================================================================
   TRIGGER ZONE
================================================================ */
typedef struct {
    AABB  bounds;
    bool  active;
    uint16_t filter_layer; /* какие слои детектируем */
    int   particles_inside[64];
    int   count;
    void (*on_enter)(struct Particle* p, void* user_data);
    void (*on_exit )(struct Particle* p, void* user_data);
    void* user_data;
} TriggerZone;

/* ================================================================
   FORCE FIELD
================================================================ */
typedef enum {
    FORCE_FIELD_ATTRACTOR,   /* притягивает к центру */
    FORCE_FIELD_REPELLER,    /* отталкивает от центра */
    FORCE_FIELD_VORTEX,      /* закручивает */
    FORCE_FIELD_DIRECTIONAL, /* постоянное направление */
    FORCE_FIELD_TURBULENCE,  /* случайные силы */
    FORCE_FIELD_DRAG,        /* торможение в зоне */
    FORCE_FIELD_WIND_ZONE,   /* направленный ветер */
} ForceFieldType;

typedef struct {
    ForceFieldType type;
    Vec2  position;
    float radius;         /* 0 = глобальное */
    float strength;
    Vec2  direction;      /* для DIRECTIONAL и WIND_ZONE */
    float falloff;        /* 0=нет, 1=линейный, 2=квадратичный */
    float time_phase;     /* внутренний таймер для turbulence */
    bool  active;
    uint16_t layer_mask;
} ForceField;

/* ================================================================
   CLOTH
================================================================ */
typedef struct {
    Particle** particles;
    int    width, height;
    Spring** springs;
    int    spring_count;
    float  structural_stiffness;
    float  shear_stiffness;
    float  bend_stiffness;
    float  damping;
    float  tear_threshold;  /* пружина рвётся при растяжении > X */
} Cloth;

/* ================================================================
   SOFT BODY
================================================================ */
typedef struct {
    Particle** particles;
    int    particle_count;
    Spring** springs;
    int    spring_count;
    float  volume;
    float  pressure;
    float  bulk_modulus;
    Vec2*  positions_old;
} SoftBody;

/* ================================================================
   PHYSICS WORLD
================================================================ */
#define MAX_TRIGGER_ZONES 16
#define MAX_FORCE_FIELDS  16
#define MAX_DIST_JOINTS   256
#define MAX_REV_JOINTS    64

typedef struct {
    /* Частицы */
    Particle** particles;
    int  particle_capacity;
    int  particle_count;

    /* Пружины */
    Spring** springs;
    int  spring_capacity;
    int  spring_count;

    /* Joints */
    DistanceJoint  dist_joints[MAX_DIST_JOINTS];
    int  dist_joint_count;
    RevoluteJoint  rev_joints[MAX_REV_JOINTS];
    int  rev_joint_count;

    /* Триггеры и силовые поля */
    TriggerZone  triggers[MAX_TRIGGER_ZONES];
    int  trigger_count;
    ForceField   force_fields[MAX_FORCE_FIELDS];
    int  force_field_count;

    /* Настройки симуляции */
    Vec2  gravity;
    float time_step;
    int   iterations;       /* PBD итерации */
    int   substeps;         /* подшаги для точности */
    bool  use_verlet;       /* Verlet вместо Euler */

    /* Ограничения мира */
    Rect  bounds;
    bool  bounds_enabled;

    /* Глобальные силы */
    Vec2  wind;
    float air_resistance;

    /* Время */
    float accumulated_time;
    float total_time;

    /* Callbacks */
    void (*on_collision)(Particle* a,Particle* b,Vec2 point,Vec2 normal,float impulse);
    void (*on_particle_death)(Particle* p);
    void (*on_spring_break)(Spring* s);

    /* Статистика */
    int  stat_collision_pairs;
    int  stat_active_particles;
} PhysicsWorld;

/* ================================================================
   СОЗДАНИЕ / УНИЧТОЖЕНИЕ
================================================================ */
PhysicsWorld* physics_world_create(Rect bounds, float time_step);
void          physics_world_destroy(PhysicsWorld* world);
void          physics_world_reset(PhysicsWorld* world); /* удалить все объекты */

/* ================================================================
   ЧАСТИЦЫ
================================================================ */
Particle* particle_create(PhysicsWorld* world, Vec2 pos, float mass, float radius, ParticleType type);
void      particle_destroy(PhysicsWorld* world, Particle* p);
void      particle_apply_force(Particle* p, Vec2 force);
void      particle_apply_impulse(Particle* p, Vec2 impulse);
void      particle_apply_force_at(Particle* p, Vec2 force, Vec2 point); /* + угловое движение */
void      particle_set_position(Particle* p, Vec2 pos); /* телепорт без физики */
void      particle_set_layers(Particle* p, uint16_t layer, uint16_t mask);
void      particle_set_lifetime(Particle* p, float seconds);
/* Взрыв — применяет импульс ко всем в радиусе */
void      physics_explosion(PhysicsWorld* world, Vec2 center, float radius, float force);

/* ================================================================
   ПРУЖИНЫ
================================================================ */
Spring* spring_create(PhysicsWorld* world, Particle* a, Particle* b, float stiffness, float damping);
Spring* spring_create_breakable(PhysicsWorld* world, Particle* a, Particle* b, float stiffness, float damping, float break_force);
void    spring_destroy(PhysicsWorld* world, Spring* s);
void    spring_set_rest_length(Spring* s, float length);

/* ================================================================
   JOINTS
================================================================ */
DistanceJoint* dist_joint_create(PhysicsWorld* world, Particle* a, Particle* b, float min_d, float max_d);
RevoluteJoint* revolute_joint_create(PhysicsWorld* world, Vec2 pivot, Particle* body);
void           dist_joint_destroy(PhysicsWorld* world, DistanceJoint* j);

/* ================================================================
   ТКАНЬ / МЯГКОЕ ТЕЛО
================================================================ */
Cloth*    cloth_create(PhysicsWorld* world, Vec2 top_left, Vec2 bottom_right, int w, int h, float mass_per_particle);
void      cloth_destroy(PhysicsWorld* world, Cloth* cloth);
void      cloth_pin(Cloth* cloth, int x, int y);
void      cloth_unpin(Cloth* cloth, int x, int y);
void      cloth_apply_wind(Cloth* cloth, Vec2 wind);
void      cloth_tear_at(PhysicsWorld* world, Cloth* cloth, Vec2 pos, float radius);
void      cloth_set_tear_threshold(Cloth* cloth, float threshold);

SoftBody* softbody_create_circle(PhysicsWorld* world, Vec2 center, float radius, int segments, float mass, float stiffness);
SoftBody* softbody_create_rectangle(PhysicsWorld* world, Vec2 mn, Vec2 mx, int sx, int sy, float mass, float stiffness);
void      softbody_destroy(PhysicsWorld* world, SoftBody* body);
Vec2      softbody_center(SoftBody* body);
void      softbody_apply_force(SoftBody* body, Vec2 force);
void      softbody_apply_pressure(SoftBody* body, PhysicsWorld* world, float dt);

/* ================================================================
   TRIGGER ZONES
================================================================ */
TriggerZone* trigger_zone_create(PhysicsWorld* world, AABB bounds);
void         trigger_zone_destroy(PhysicsWorld* world, TriggerZone* t);
bool         trigger_zone_has_particle(TriggerZone* t, Particle* p);

/* ================================================================
   FORCE FIELDS
================================================================ */
ForceField* force_field_create(PhysicsWorld* world, ForceFieldType type, Vec2 pos, float radius, float strength);
void        force_field_destroy(PhysicsWorld* world, ForceField* f);
void        force_field_set_direction(ForceField* f, Vec2 dir);

/* ================================================================
   RAYCASTING
================================================================ */
RaycastHit physics_raycast(PhysicsWorld* world, Vec2 origin, Vec2 direction, float max_dist, uint16_t layer_mask);
int        physics_overlap_circle(PhysicsWorld* world, Vec2 center, float radius, uint16_t layer_mask, Particle** out, int max_out);
int        physics_overlap_rect(PhysicsWorld* world, AABB bounds, uint16_t layer_mask, Particle** out, int max_out);

/* ================================================================
   ОБНОВЛЕНИЕ
================================================================ */
void physics_world_update(PhysicsWorld* world);
void physics_world_update_springs(PhysicsWorld* world);
void physics_world_update_joints(PhysicsWorld* world);
void physics_world_update_force_fields(PhysicsWorld* world, float dt);
void physics_world_update_triggers(PhysicsWorld* world);
void physics_world_check_collisions(PhysicsWorld* world);
void physics_world_resolve_collisions(PhysicsWorld* world);

/* ================================================================
   УТИЛИТЫ
================================================================ */
void  physics_world_set_gravity(PhysicsWorld* world, Vec2 gravity);
void  physics_world_set_substeps(PhysicsWorld* world, int substeps);
void  physics_world_set_iterations(PhysicsWorld* world, int iters);
void  physics_world_add_force_region(PhysicsWorld* world, Rect region, Vec2 force);
void  physics_world_clear_forces(PhysicsWorld* world);
float physics_world_kinetic_energy(PhysicsWorld* world);
Vec2  physics_world_center_of_mass(PhysicsWorld* world);

#endif /* PHYSIK_H */
