/* hitboxes.h — Cngine v2
 * LGPL v3 | Chikatilo / sooborn / SGLauncher
 *
 * Hitbox system: AABB, Circle, OBB, Capsule, Composite
 * Правильная трансформация, debug-рендер, overlap/sweep-тесты
 */
#ifndef HITBOXES_H
#define HITBOXES_H

#include "engine_math.h"
#include "render.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/* ================================================================
   ТИПЫ ХИТБОКСОВ
================================================================ */
typedef enum {
    HB_AABB    = 0,
    HB_CIRCLE  = 1,
    HB_OBB     = 2,
    HB_CAPSULE = 3,
    HB_POLYGON = 4,
} HitboxShape;

/* Максимум вершин для полигонального хитбокса */
#define HB_MAX_POLY_VERTS 16
/* Максимум дочерних в составном хитбоксе */
#define HB_MAX_CHILDREN 8

/* ================================================================
   РЕЗУЛЬТАТ КОЛЛИЗИИ
================================================================ */
typedef struct {
    bool  hit;
    Vec2  point;      /* точка контакта (центр пересечения) */
    Vec2  normal;     /* нормаль: направление разрешения для A */
    float depth;      /* глубина проникновения */
    float t;          /* время контакта при sweep-тесте [0,1] */
} HitResult;

/* ================================================================
   ХИТБОКС
================================================================ */
typedef struct Hitbox Hitbox;

struct Hitbox {
    HitboxShape shape;

    /* Локальное смещение от "владельца" */
    Vec2  offset;
    float rotation; /* радианы, для OBB / Capsule / Polygon */

    /* Параметры формы */
    union {
        struct { float w, h; }         aabb;    /* half-extents */
        struct { float radius; }       circle;
        struct { float w, h; }         obb;     /* half-extents */
        struct { float radius, len; }  capsule; /* radius, half-length вдоль оси */
        struct {
            Vec2 verts[HB_MAX_POLY_VERTS];
            int  count;
        }                              polygon;
    };

    /* Слой и маска */
    uint16_t layer;
    uint16_t mask;

    /* Состояние */
    bool     active;
    bool     is_trigger;  /* только детектирование, без физического разрешения */

    /* Для составного хитбокса */
    Hitbox*  children[HB_MAX_CHILDREN];
    int      child_count;

    /* Пользовательские данные */
    void*    owner;       /* указатель на объект-владелец */
    int      id;          /* пользовательский ID */

    /* Callback: вызывается при коллизии */
    void (*on_collision)(Hitbox* self, Hitbox* other, HitResult result, void* ud);
    void* callback_ud;
};

/* ================================================================
   ФАБРИКА
================================================================ */
Hitbox* hb_create_aabb   (float half_w, float half_h);
Hitbox* hb_create_circle (float radius);
Hitbox* hb_create_obb    (float half_w, float half_h);
Hitbox* hb_create_capsule(float radius, float half_len);
Hitbox* hb_create_polygon(Vec2* verts, int count);
Hitbox* hb_create_composite(void);

void    hb_destroy(Hitbox* hb);
void    hb_composite_add(Hitbox* parent, Hitbox* child);

/* ================================================================
   ТРАНСФОРМАЦИЯ
   world_pos — позиция владельца, world_rot — вращение владельца
================================================================ */
/* Получить AABB хитбокса в мировых координатах */
AABB    hb_world_aabb  (const Hitbox* hb, Vec2 world_pos, float world_rot);
/* Получить центр хитбокса в мировых координатах */
Vec2    hb_world_center(const Hitbox* hb, Vec2 world_pos, float world_rot);

/* ================================================================
   ТЕСТЫ ПЕРЕСЕЧЕНИЯ (stationary)
================================================================ */
HitResult hb_test(const Hitbox* a, Vec2 pos_a, float rot_a,
                  const Hitbox* b, Vec2 pos_b, float rot_b);

/* Проверка, содержит ли хитбокс точку */
bool hb_contains_point(const Hitbox* hb, Vec2 world_pos, float world_rot, Vec2 point);

/* Raycast против хитбокса */
bool hb_raycast(const Hitbox* hb, Vec2 world_pos, float world_rot,
                Vec2 ray_origin, Vec2 ray_dir, float max_dist, float* out_t);

/* ================================================================
   SWEEP (continuous collision detection)
   delta — вектор перемещения A за кадр
================================================================ */
HitResult hb_sweep(const Hitbox* a, Vec2 pos_a, float rot_a, Vec2 delta,
                   const Hitbox* b, Vec2 pos_b, float rot_b);

/* ================================================================
   МЕНЕДЖЕР ХИТБОКСОВ
================================================================ */
#define HBM_MAX 1024

typedef struct {
    Hitbox* boxes[HBM_MAX];
    Vec2    positions[HBM_MAX];
    float   rotations[HBM_MAX];
    int     count;
    int     ids[HBM_MAX];   /* идентификатор владельца */
} HitboxManager;

HitboxManager* hbm_create(void);
void           hbm_destroy(HitboxManager* mgr);
int            hbm_add(HitboxManager* mgr, Hitbox* hb, Vec2 pos, float rot);
void           hbm_remove(HitboxManager* mgr, int idx);
void           hbm_update_transform(HitboxManager* mgr, int idx, Vec2 pos, float rot);
/* Обновить все коллизии и вызвать callbacks */
void           hbm_process(HitboxManager* mgr);
/* Найти все пересечения с данным хитбоксом */
int            hbm_query_hitbox(HitboxManager* mgr, const Hitbox* query, Vec2 qpos, float qrot,
                                int* out_indices, HitResult* out_results, int max_out);
/* Найти все в AABB */
int            hbm_query_aabb(HitboxManager* mgr, AABB region,
                               int* out_indices, int max_out);
/* Raycast по всем */
int            hbm_raycast(HitboxManager* mgr, Vec2 origin, Vec2 dir, float max_dist,
                            uint16_t layer_mask, int* out_idx, float* out_t);

/* ================================================================
   DEBUG RENDER
================================================================ */
void hb_debug_draw(Renderer* r, const Hitbox* hb, Vec2 world_pos, float world_rot,
                   Color active_col, Color trigger_col, bool filled);
void hbm_debug_draw_all(Renderer* r, const HitboxManager* mgr,
                        Color active_col, Color trigger_col);

/* ================================================================
   УТИЛИТЫ
================================================================ */
/* Создать хитбокс под частицу physik.h */
Hitbox* hb_from_particle_radius(float radius);

/* Масштабирование хитбокса */
void hb_scale(Hitbox* hb, float sx, float sy);

#endif /* HITBOXES_H */
