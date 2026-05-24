/* postfx_config.h — Cngine v2
 * LGPL v3 | Chikatilo / sooborn / SGLauncher
 *
 * Управление пост-обработкой: включать/выключать отдельные эффекты,
 * умный блум только для помеченных объектов, пресеты.
 *
 * Подключи в своей game.c:
 *   #include "engine/postfx_config.h"
 *   // опционально — отключить всю постобработку вообще:
 *   // #define CNGINE_NO_POSTFX
 */
#ifndef POSTFX_CONFIG_H
#define POSTFX_CONFIG_H

#include "render.h"
#include <stdbool.h>

/* ================================================================
   ФЛАГИ КОМПИЛЯЦИИ
   Определить ДО включения этого файла, чтобы полностью убрать из
   бинарника соответствующие эффекты.
================================================================ */
/* #define CNGINE_NO_POSTFX          // выключить всё */
/* #define CNGINE_NO_BLOOM           */
/* #define CNGINE_NO_VIGNETTE        */
/* #define CNGINE_NO_CHROMAB         */
/* #define CNGINE_NO_SCANLINES       */
/* #define CNGINE_NO_FILMGRAIN       */
/* #define CNGINE_NO_COLORGRADE      */
/* #define CNGINE_NO_CRT             */
/* #define CNGINE_NO_HEATWAVE        */
/* #define CNGINE_NO_UNDERWATER      */
/* #define CNGINE_NO_NIGHTVISION     */
/* #define CNGINE_NO_DRUNK           */

/* ================================================================
   УМНЫЙ BLOOM — только для помеченных пикселей
================================================================ */
/* Порог яркости: пиксели ярче этого значения будут "светиться"
   даже без явной пометки (автоматический режим) */
#define SMART_BLOOM_AUTO_THRESHOLD 0.85f

/* Маска альфа-канала: если alpha == BLOOM_ALPHA_TAG, пиксель
   принудительно участвует в bloom (ручная пометка).
   Используй pb_put_bloom() вместо pb_put() для таких объектов. */
#define BLOOM_ALPHA_TAG 0xFE

/* Вспомогательная функция: нарисовать пиксель, помеченный для bloom */
static inline void pb_put_bloom(Renderer* r, int x, int y, Color c){
    c.a = BLOOM_ALPHA_TAG;
    pb_put(r, x, y, c);
}

/* ================================================================
   ПРЕСЕТЫ ПОСТОБРАБОТКИ
================================================================ */
typedef enum {
    PFX_PRESET_NONE      = 0,
    PFX_PRESET_RETRO,         /* scanlines + pixelate + chromatic */
    PFX_PRESET_CINEMATIC,     /* vignette + color_grade + grain */
    PFX_PRESET_HORROR,        /* vignette+grain+desaturate+drunk */
    PFX_PRESET_UNDERWATER,    /* underwater + chromatic */
    PFX_PRESET_NIGHT,         /* night_vision */
    PFX_PRESET_NEON,          /* bloom(heavy) + color_grade(contrast) */
    PFX_PRESET_DREAM,         /* bloom + chromatic + drunk(light) */
    PFX_PRESET_CUSTOM,        /* ручные настройки */
} PostFXPreset;

/* Применить пресет к Renderer */
void postfx_apply_preset(Renderer* r, PostFXPreset preset);

/* ================================================================
   ВКЛЮЧЕНИЕ/ВЫКЛЮЧЕНИЕ ОТДЕЛЬНЫХ ЭФФЕКТОВ
================================================================ */
void postfx_enable_bloom(Renderer* r, bool on, int radius, float strength, float threshold);
void postfx_enable_vignette(Renderer* r, bool on, float strength, Color color);
void postfx_enable_chromatic(Renderer* r, bool on, float strength);
void postfx_enable_scanlines(Renderer* r, bool on, float alpha, int gap);
void postfx_enable_grain(Renderer* r, bool on, float strength);
void postfx_enable_colorgrade(Renderer* r, bool on,
                               float saturation, float contrast,
                               float brightness, float gamma, Color tint);
void postfx_enable_crt(Renderer* r, bool on, float curvature);
void postfx_enable_heat(Renderer* r, bool on, float intensity, float speed);
void postfx_enable_underwater(Renderer* r, bool on, float distort, Color tint);
void postfx_enable_nightvision(Renderer* r, bool on, float noise);
void postfx_enable_drunk(Renderer* r, bool on, float amount);
void postfx_enable_pixelate(Renderer* r, bool on, int pixel_size);

/* Выключить ВСЕ эффекты */
void postfx_disable_all(Renderer* r);

/* ================================================================
   УМНЫЙ BLOOM — независимый проход
   Вызывать ПОСЛЕ рендера сцены, ДО renderer_present()
   Блум применяется только к:
     1) пикселям помеченным BLOOM_ALPHA_TAG
     2) пикселям ярче SMART_BLOOM_AUTO_THRESHOLD (если auto=true)
================================================================ */
void postfx_smart_bloom(Renderer* r, int radius, float strength, bool auto_bright);

/* ================================================================
   АНИМАЦИЯ ЭФФЕКТОВ (плавное включение/выключение)
================================================================ */
typedef struct {
    PostFX    target;       /* целевые значения */
    PostFX    current;      /* текущие значения */
    float     blend;        /* 0..1 */
    float     blend_speed;  /* скорость перехода */
    bool      transitioning;
} PostFXAnimator;

PostFXAnimator* postfx_animator_create(Renderer* r);
void            postfx_animator_destroy(PostFXAnimator* anim);
void            postfx_animator_set_target(PostFXAnimator* anim, PostFXPreset preset);
void            postfx_animator_update(PostFXAnimator* anim, Renderer* r, float dt);

#endif /* POSTFX_CONFIG_H */
