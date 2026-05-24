/* audio.h — Cngine v2
 * LGPL v3 | Chikatilo / sooborn / SGLauncher
 *
 * Аудио-система на базе SDL2 audio + простой software mixer
 * Возможности:
 *   - Фоновая музыка (loop, fade in/out, crossfade)
 *   - Звуковые эффекты по тригеру (именованные, позиционные)
 *   - Пулинг каналов (до 32 одновременных звуков)
 *   - Громкость: master, music, sfx
 *   - Pitch, pan, fade
 *   - Именованные события (trigger_sound_event)
 *   - WAV-загрузка (встроенная через SDL2)
 */
#ifndef AUDIO_H
#define AUDIO_H

#include "sdl/SDL2/SDL.h"
#include <stdbool.h>
#include <stdint.h>

/* ================================================================
   КОНФИГУРАЦИЯ
================================================================ */
#define AUDIO_MAX_CHANNELS  32      /* одновременных SFX */
#define AUDIO_MAX_SOUNDS    128     /* загруженных звуков */
#define AUDIO_MAX_MUSIC     8       /* загруженной музыки */
#define AUDIO_MAX_EVENTS    64      /* именованных событий */
#define AUDIO_SAMPLE_RATE   44100
#define AUDIO_CHANNELS      2       /* stereo */
#define AUDIO_BUFFER_SIZE   1024    /* сэмплов */

/* ================================================================
   ЗВУКОВОЙ БУФЕР
================================================================ */
typedef struct {
    Uint8*  data;       /* PCM данные (SDL_AudioFormat SDL_AUDIO_S16) */
    Uint32  length;     /* байт */
    int     sample_rate;
    int     channels;
    char    name[64];   /* идентификатор */
    bool    loaded;
} Sound;

/* ================================================================
   МУЗЫКАЛЬНЫЙ ТРЕК
================================================================ */
typedef struct {
    Sound* sound;
    float  volume;      /* 0..1 */
    float  pitch;
    bool   looping;
    bool   loaded;
    char   name[64];
} MusicTrack;

/* ================================================================
   КАНАЛ ВОСПРОИЗВЕДЕНИЯ
================================================================ */
typedef struct {
    Sound*  sound;
    Uint32  position;   /* байтовая позиция */
    float   volume;     /* 0..1 */
    float   pan;        /* -1..1 (L..R) */
    float   pitch;      /* 0.5..2.0 */
    bool    looping;
    bool    active;
    float   fade_volume; /* текущая огибающая */
    float   fade_target;
    float   fade_speed;  /* скорость fade (1/сек) */
    /* 2D аудио */
    bool    positional;
    float   world_x, world_y;
    float   listener_x, listener_y;
    float   max_dist;
    /* Тег для останова по группе */
    int     group;      /* 0=sfx, 1=music, 2=ambient, 3=ui */
} AudioChannel;

/* ================================================================
   ИМЕНОВАННОЕ СОБЫТИЕ
================================================================ */
typedef struct {
    char  trigger_name[64];
    char  sound_name[64];
    float volume_min, volume_max;
    float pitch_min, pitch_max;
    float pan;
    int   group;
    bool  looping;
    float cooldown;     /* минимальный интервал между воспроизведениями */
    float _last_played;
} AudioEvent;

/* ================================================================
   АУДИО СИСТЕМА
================================================================ */
typedef struct {
    /* SDL */
    SDL_AudioDeviceID device;
    SDL_AudioSpec     spec;

    /* Звуки и музыка */
    Sound       sounds[AUDIO_MAX_SOUNDS];
    int         sound_count;
    MusicTrack  music[AUDIO_MAX_MUSIC];
    int         music_count;

    /* Каналы */
    AudioChannel channels[AUDIO_MAX_CHANNELS];

    /* Текущая музыка */
    int   current_music;  /* индекс в channels (-1 если нет) */
    int   next_music;     /* для crossfade (-1 если нет) */
    float crossfade_time;
    float crossfade_timer;

    /* Громкость */
    float master_volume;  /* 0..1 */
    float music_volume;
    float sfx_volume;
    float ambient_volume;
    float ui_volume;

    /* Позиция слушателя */
    float listener_x, listener_y;
    float listener_range;   /* радиус слышимости */

    /* События */
    AudioEvent events[AUDIO_MAX_EVENTS];
    int        event_count;

    /* Время */
    float      time;        /* для cooldown */

    /* Состояние */
    bool       initialized;
    bool       muted;
    bool       paused_all;

    /* Внутренний буфер микшера */
    float      mix_buf[AUDIO_BUFFER_SIZE * AUDIO_CHANNELS];
    SDL_mutex* lock;
} AudioSystem;

/* ================================================================
   ИНИЦИАЛИЗАЦИЯ / УНИЧТОЖЕНИЕ
================================================================ */
AudioSystem* audio_create(void);
void         audio_destroy(AudioSystem* a);
void         audio_update(AudioSystem* a, float dt);  /* вызывать каждый кадр */

/* ================================================================
   ЗАГРУЗКА ЗВУКОВ
================================================================ */
/* Загрузить WAV-файл, вернуть индекс или -1 */
int  audio_load_sound(AudioSystem* a, const char* path, const char* name);
/* Загрузить музыкальный трек */
int  audio_load_music(AudioSystem* a, const char* path, const char* name);
/* Выгрузить звук по имени */
void audio_unload_sound(AudioSystem* a, const char* name);
void audio_unload_all(AudioSystem* a);

/* ================================================================
   ВОСПРОИЗВЕДЕНИЕ SFX
================================================================ */
/* Сыграть звук по имени. Возвращает ID канала или -1 */
int  audio_play(AudioSystem* a, const char* sound_name, float volume, float pitch, float pan);
/* Позиционный звук в мировых координатах */
int  audio_play_at(AudioSystem* a, const char* sound_name, float wx, float wy, float volume);
/* Остановить канал */
void audio_stop_channel(AudioSystem* a, int channel_id);
/* Остановить все каналы группы */
void audio_stop_group(AudioSystem* a, int group);
/* Fade out канала */
void audio_fadeout_channel(AudioSystem* a, int channel_id, float seconds);
/* Поставить на паузу / снять */
void audio_pause_channel(AudioSystem* a, int channel_id, bool paused);

/* ================================================================
   МУЗЫКА
================================================================ */
void audio_play_music(AudioSystem* a, const char* music_name, float volume, bool loop);
void audio_stop_music(AudioSystem* a);
void audio_pause_music(AudioSystem* a, bool paused);
void audio_set_music_volume(AudioSystem* a, float volume);
/* Crossfade к новой музыке */
void audio_crossfade_music(AudioSystem* a, const char* music_name, float fade_seconds);
/* Fade out и стоп */
void audio_fadeout_music(AudioSystem* a, float seconds);
/* Мгновенный seek */
void audio_music_seek(AudioSystem* a, float seconds);

/* ================================================================
   ГЛОБАЛЬНЫЕ НАСТРОЙКИ
================================================================ */
void audio_set_master_volume(AudioSystem* a, float v);
void audio_set_sfx_volume(AudioSystem* a, float v);
void audio_set_ambient_volume(AudioSystem* a, float v);
void audio_set_ui_volume(AudioSystem* a, float v);
void audio_mute(AudioSystem* a, bool muted);
void audio_pause_all(AudioSystem* a, bool paused);

/* ================================================================
   2D ПОЗИЦИОННОЕ АУДИО
================================================================ */
void audio_set_listener(AudioSystem* a, float wx, float wy, float range);
void audio_update_channel_position(AudioSystem* a, int channel_id, float wx, float wy);

/* ================================================================
   СОБЫТИЯ (ТРИГГЕРЫ)
================================================================ */
/* Зарегистрировать именованное событие */
void audio_register_event(AudioSystem* a, const char* trigger,
                          const char* sound_name,
                          float vol_min, float vol_max,
                          float pitch_min, float pitch_max,
                          float cooldown);
/* Сыграть событие по имени триггера */
int  audio_trigger(AudioSystem* a, const char* trigger_name);
/* Позиционный триггер */
int  audio_trigger_at(AudioSystem* a, const char* trigger_name, float wx, float wy);

/* ================================================================
   УТИЛИТЫ
================================================================ */
bool  audio_is_playing(AudioSystem* a, int channel_id);
float audio_channel_progress(AudioSystem* a, int channel_id); /* 0..1 */
int   audio_active_channels(AudioSystem* a);
Sound* audio_find_sound(AudioSystem* a, const char* name);

#endif /* AUDIO_H */
