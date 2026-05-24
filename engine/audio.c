/* audio.c — Cngine v2
 * LGPL v3 | Chikatilo / sooborn / SGLauncher
 */
#include "audio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ================================================================
   ВНУТРЕННИЕ УТИЛИТЫ
================================================================ */
static float _clampf(float v,float lo,float hi){ return v<lo?lo:(v>hi?hi:v); }
static float _lerpf(float a,float b,float t){ return a+(b-a)*t; }

static int _find_sound(AudioSystem* a, const char* name){
    for(int i=0;i<a->sound_count;i++)
        if(a->sounds[i].loaded && strcmp(a->sounds[i].name,name)==0) return i;
    return -1;
}
static int _find_music(AudioSystem* a, const char* name){
    for(int i=0;i<a->music_count;i++)
        if(a->music[i].loaded && strcmp(a->music[i].name,name)==0) return i;
    return -1;
}
static int _free_channel(AudioSystem* a){
    for(int i=0;i<AUDIO_MAX_CHANNELS;i++)
        if(!a->channels[i].active) return i;
    return -1;
}
/* Вычислить громкость канала с учётом позиции */
static void _calc_pan_vol(AudioChannel* ch, float* out_vol, float* out_l, float* out_r){
    float vol = ch->volume * ch->fade_volume;
    if(ch->positional){
        float dx=ch->world_x-ch->listener_x, dy=ch->world_y-ch->listener_y;
        float dist=sqrtf(dx*dx+dy*dy);
        float att=(ch->max_dist>0.f)? 1.f-_clampf(dist/ch->max_dist,0.f,1.f): 1.f;
        vol*=att;
        float pan=_clampf(dx/(ch->max_dist+0.01f), -1.f, 1.f);
        ch->pan=pan;
    }
    *out_vol=vol;
    float p=_clampf(ch->pan,-1.f,1.f);
    *out_l=(p<=0.f)?1.f:(1.f-p);
    *out_r=(p>=0.f)?1.f:(1.f+p);
}

/* ================================================================
   SDL AUDIO CALLBACK
================================================================ */
static void _audio_callback(void* userdata, Uint8* stream, int len){
    AudioSystem* a=(AudioSystem*)userdata;
    int samples=len/sizeof(Sint16);
    int frames=samples/AUDIO_CHANNELS;

    /* Очистить */
    float mix[AUDIO_BUFFER_SIZE*AUDIO_CHANNELS];
    memset(mix,0,sizeof(float)*frames*AUDIO_CHANNELS);

    SDL_LockMutex(a->lock);
    if(!a->paused_all){
        for(int c=0;c<AUDIO_MAX_CHANNELS;c++){
            AudioChannel* ch=&a->channels[c];
            if(!ch->active||!ch->sound) continue;

            float group_vol=1.f;
            switch(ch->group){
                case 0: group_vol=a->sfx_volume; break;
                case 1: group_vol=a->music_volume; break;
                case 2: group_vol=a->ambient_volume; break;
                case 3: group_vol=a->ui_volume; break;
            }
            float vol,lv,rv;
            _calc_pan_vol(ch,&vol,&lv,&rv);
            vol*=a->master_volume*group_vol;
            if(a->muted) vol=0.f;

            Sound* snd=ch->sound;
            Sint16* src=(Sint16*)snd->data;
            int src_frames=(int)(snd->length/sizeof(Sint16)/snd->channels);

            for(int f=0;f<frames;f++){
                if(!ch->active) break;
                int pos=(int)(ch->position/sizeof(Sint16));
                if(pos>=src_frames*(snd->channels)){
                    if(ch->looping){ ch->position=0; pos=0; }
                    else{ ch->active=false; break; }
                }
                float s_l=(float)src[pos]   /32768.f;
                float s_r=(snd->channels>1)?(float)src[pos+1]/32768.f:s_l;
                mix[f*2+0]+=s_l*vol*lv;
                mix[f*2+1]+=s_r*vol*rv;
                ch->position+=sizeof(Sint16)*snd->channels;
            }
        }
    }
    SDL_UnlockMutex(a->lock);

    /* Запись в поток */
    Sint16* out=(Sint16*)stream;
    for(int i=0;i<frames*AUDIO_CHANNELS;i++){
        float v=_clampf(mix[i],-1.f,1.f);
        out[i]=(Sint16)(v*32767.f);
    }
}

/* ================================================================
   СОЗДАНИЕ / УНИЧТОЖЕНИЕ
================================================================ */
AudioSystem* audio_create(void){
    if(SDL_InitSubSystem(SDL_INIT_AUDIO)<0){
        printf("[Audio] SDL_InitSubSystem failed: %s\n",SDL_GetError());
        return NULL;
    }
    AudioSystem* a=(AudioSystem*)calloc(1,sizeof(AudioSystem));
    if(!a) return NULL;

    a->lock=SDL_CreateMutex();
    a->master_volume=1.f;
    a->music_volume=1.f;
    a->sfx_volume=1.f;
    a->ambient_volume=1.f;
    a->ui_volume=1.f;
    a->current_music=-1;
    a->next_music=-1;
    a->listener_range=500.f;

    SDL_AudioSpec desired={0};
    desired.freq=AUDIO_SAMPLE_RATE;
    desired.format=AUDIO_S16;
    desired.channels=AUDIO_CHANNELS;
    desired.samples=AUDIO_BUFFER_SIZE;
    desired.callback=_audio_callback;
    desired.userdata=a;

    a->device=SDL_OpenAudioDevice(NULL,0,&desired,&a->spec,0);
    if(!a->device){
        printf("[Audio] SDL_OpenAudioDevice failed: %s\n",SDL_GetError());
        SDL_DestroyMutex(a->lock);
        free(a);
        return NULL;
    }
    SDL_PauseAudioDevice(a->device,0);
    a->initialized=true;
    printf("[Audio] Initialized OK (device %d)\n",a->device);
    return a;
}

void audio_destroy(AudioSystem* a){
    if(!a) return;
    if(a->device) SDL_CloseAudioDevice(a->device);
    audio_unload_all(a);
    if(a->lock) SDL_DestroyMutex(a->lock);
    free(a);
}

void audio_update(AudioSystem* a, float dt){
    if(!a) return;
    a->time+=dt;

    SDL_LockMutex(a->lock);
    /* Fade каналов */
    for(int i=0;i<AUDIO_MAX_CHANNELS;i++){
        AudioChannel* ch=&a->channels[i];
        if(!ch->active) continue;
        if(ch->fade_volume!=ch->fade_target){
            float step=ch->fade_speed*dt;
            if(ch->fade_volume<ch->fade_target)
                ch->fade_volume=fminf(ch->fade_volume+step,ch->fade_target);
            else
                ch->fade_volume=fmaxf(ch->fade_volume-step,ch->fade_target);
            if(ch->fade_volume<=0.f && ch->fade_target<=0.f)
                ch->active=false;
        }
    }
    /* Crossfade музыки */
    if(a->next_music>=0 && a->crossfade_time>0.f){
        a->crossfade_timer+=dt;
        float t=_clampf(a->crossfade_timer/a->crossfade_time,0.f,1.f);
        if(a->current_music>=0){
            a->channels[a->current_music].fade_volume=_lerpf(1.f,0.f,t);
        }
        a->channels[a->next_music].fade_volume=_lerpf(0.f,1.f,t);
        if(t>=1.f){
            if(a->current_music>=0) a->channels[a->current_music].active=false;
            a->current_music=a->next_music;
            a->next_music=-1;
        }
    }
    SDL_UnlockMutex(a->lock);
}

/* ================================================================
   ЗАГРУЗКА ЗВУКОВ
================================================================ */
int audio_load_sound(AudioSystem* a, const char* path, const char* name){
    if(!a||a->sound_count>=AUDIO_MAX_SOUNDS) return -1;
    SDL_AudioSpec spec; Uint8* buf; Uint32 len;
    if(!SDL_LoadWAV(path,&spec,&buf,&len)){
        printf("[Audio] Failed to load '%s': %s\n",path,SDL_GetError());
        return -1;
    }
    /* Конвертировать в наш формат */
    SDL_AudioCVT cvt;
    SDL_BuildAudioCVT(&cvt,spec.format,spec.channels,spec.freq,
                       AUDIO_S16,AUDIO_CHANNELS,AUDIO_SAMPLE_RATE);
    cvt.len=(int)len; cvt.buf=(Uint8*)malloc(len*(size_t)(cvt.len_mult+1));
    memcpy(cvt.buf,buf,len);
    SDL_FreeWAV(buf);
    SDL_ConvertAudio(&cvt);

    Sound* s=&a->sounds[a->sound_count];
    s->data=cvt.buf;
    s->length=(Uint32)cvt.len_cvt;
    s->sample_rate=AUDIO_SAMPLE_RATE;
    s->channels=AUDIO_CHANNELS;
    strncpy(s->name,name,63);
    s->loaded=true;
    printf("[Audio] Loaded sound '%s'\n",name);
    return a->sound_count++;
}

int audio_load_music(AudioSystem* a, const char* path, const char* name){
    int idx=audio_load_sound(a,path,name);
    if(idx<0) return -1;
    if(a->music_count>=AUDIO_MAX_MUSIC) return -1;
    MusicTrack* m=&a->music[a->music_count];
    m->sound=&a->sounds[idx];
    m->volume=1.f; m->pitch=1.f; m->looping=true;
    strncpy(m->name,name,63); m->loaded=true;
    return a->music_count++;
}

void audio_unload_sound(AudioSystem* a, const char* name){
    int idx=_find_sound(a,name);
    if(idx<0) return;
    SDL_LockMutex(a->lock);
    free(a->sounds[idx].data);
    memset(&a->sounds[idx],0,sizeof(Sound));
    SDL_UnlockMutex(a->lock);
}

void audio_unload_all(AudioSystem* a){
    if(!a) return;
    SDL_LockMutex(a->lock);
    for(int i=0;i<AUDIO_MAX_CHANNELS;i++) a->channels[i].active=false;
    for(int i=0;i<a->sound_count;i++){
        if(a->sounds[i].data) free(a->sounds[i].data);
    }
    memset(a->sounds,0,sizeof(a->sounds));
    a->sound_count=0; a->music_count=0;
    SDL_UnlockMutex(a->lock);
}

/* ================================================================
   ВОСПРОИЗВЕДЕНИЕ SFX
================================================================ */
int audio_play(AudioSystem* a, const char* sound_name, float volume, float pitch, float pan){
    if(!a) return -1;
    int si=_find_sound(a,sound_name);
    if(si<0){ printf("[Audio] Sound '%s' not found\n",sound_name); return -1; }
    int ci=_free_channel(a);
    if(ci<0) return -1;

    SDL_LockMutex(a->lock);
    AudioChannel* ch=&a->channels[ci];
    memset(ch,0,sizeof(AudioChannel));
    ch->sound=&a->sounds[si];
    ch->position=0;
    ch->volume=_clampf(volume,0.f,1.f);
    ch->pitch=_clampf(pitch,0.1f,4.f);
    ch->pan=_clampf(pan,-1.f,1.f);
    ch->fade_volume=1.f;
    ch->fade_target=1.f;
    ch->active=true;
    ch->group=0;
    SDL_UnlockMutex(a->lock);
    return ci;
}

int audio_play_at(AudioSystem* a, const char* sound_name, float wx, float wy, float volume){
    int ci=audio_play(a,sound_name,volume,1.f,0.f);
    if(ci<0) return -1;
    SDL_LockMutex(a->lock);
    AudioChannel* ch=&a->channels[ci];
    ch->positional=true;
    ch->world_x=wx; ch->world_y=wy;
    ch->listener_x=a->listener_x; ch->listener_y=a->listener_y;
    ch->max_dist=a->listener_range;
    SDL_UnlockMutex(a->lock);
    return ci;
}

void audio_stop_channel(AudioSystem* a, int ci){
    if(!a||ci<0||ci>=AUDIO_MAX_CHANNELS) return;
    SDL_LockMutex(a->lock);
    a->channels[ci].active=false;
    SDL_UnlockMutex(a->lock);
}

void audio_stop_group(AudioSystem* a, int group){
    if(!a) return;
    SDL_LockMutex(a->lock);
    for(int i=0;i<AUDIO_MAX_CHANNELS;i++)
        if(a->channels[i].active && a->channels[i].group==group)
            a->channels[i].active=false;
    SDL_UnlockMutex(a->lock);
}

void audio_fadeout_channel(AudioSystem* a, int ci, float seconds){
    if(!a||ci<0||ci>=AUDIO_MAX_CHANNELS) return;
    SDL_LockMutex(a->lock);
    AudioChannel* ch=&a->channels[ci];
    ch->fade_target=0.f;
    ch->fade_speed=(seconds>0.f)?1.f/seconds:100.f;
    SDL_UnlockMutex(a->lock);
}

void audio_pause_channel(AudioSystem* a, int ci, bool paused){
    if(!a||ci<0||ci>=AUDIO_MAX_CHANNELS) return;
    /* простая реализация: приглушить до нуля */
    (void)paused;
}

/* ================================================================
   МУЗЫКА
================================================================ */
void audio_play_music(AudioSystem* a, const char* name, float volume, bool loop){
    if(!a) return;
    int mi=_find_music(a,name);
    if(mi<0){ printf("[Audio] Music '%s' not found\n",name); return; }
    /* остановить текущую */
    if(a->current_music>=0){
        SDL_LockMutex(a->lock);
        a->channels[a->current_music].active=false;
        SDL_UnlockMutex(a->lock);
    }
    int ci=_free_channel(a);
    if(ci<0) return;
    SDL_LockMutex(a->lock);
    AudioChannel* ch=&a->channels[ci];
    memset(ch,0,sizeof(AudioChannel));
    ch->sound=a->music[mi].sound;
    ch->volume=_clampf(volume,0.f,1.f);
    ch->looping=loop;
    ch->active=true;
    ch->group=1;
    ch->fade_volume=1.f;
    ch->fade_target=1.f;
    a->current_music=ci;
    SDL_UnlockMutex(a->lock);
}

void audio_stop_music(AudioSystem* a){
    if(!a||a->current_music<0) return;
    audio_stop_channel(a,a->current_music);
    a->current_music=-1;
}

void audio_pause_music(AudioSystem* a, bool paused){
    (void)a;(void)paused;
}

void audio_set_music_volume(AudioSystem* a, float v){
    if(!a) return;
    a->music_volume=_clampf(v,0.f,1.f);
}

void audio_crossfade_music(AudioSystem* a, const char* name, float fade_secs){
    if(!a) return;
    int mi=_find_music(a,name);
    if(mi<0) return;
    int ci=_free_channel(a);
    if(ci<0) return;
    SDL_LockMutex(a->lock);
    AudioChannel* ch=&a->channels[ci];
    memset(ch,0,sizeof(AudioChannel));
    ch->sound=a->music[mi].sound;
    ch->volume=1.f; ch->looping=true; ch->active=true; ch->group=1;
    ch->fade_volume=0.f; ch->fade_target=1.f;
    a->next_music=ci;
    a->crossfade_time=fade_secs;
    a->crossfade_timer=0.f;
    SDL_UnlockMutex(a->lock);
}

void audio_fadeout_music(AudioSystem* a, float seconds){
    if(!a||a->current_music<0) return;
    audio_fadeout_channel(a,a->current_music,seconds);
    a->current_music=-1;
}

void audio_music_seek(AudioSystem* a, float seconds){
    if(!a||a->current_music<0) return;
    AudioChannel* ch=&a->channels[a->current_music];
    if(!ch->sound) return;
    Uint32 byte_pos=(Uint32)(seconds*(float)AUDIO_SAMPLE_RATE*sizeof(Sint16)*AUDIO_CHANNELS);
    ch->position=(byte_pos<ch->sound->length)?byte_pos:0;
}

/* ================================================================
   ГЛОБАЛЬНЫЕ НАСТРОЙКИ
================================================================ */
void audio_set_master_volume(AudioSystem* a,float v){ if(a)a->master_volume=_clampf(v,0.f,1.f); }
void audio_set_sfx_volume(AudioSystem* a,float v)   { if(a)a->sfx_volume=_clampf(v,0.f,1.f); }
void audio_set_ambient_volume(AudioSystem* a,float v){ if(a)a->ambient_volume=_clampf(v,0.f,1.f); }
void audio_set_ui_volume(AudioSystem* a,float v)     { if(a)a->ui_volume=_clampf(v,0.f,1.f); }
void audio_mute(AudioSystem* a,bool m)               { if(a)a->muted=m; }
void audio_pause_all(AudioSystem* a,bool p)          { if(a)a->paused_all=p; }

/* ================================================================
   2D ПОЗИЦИОННОЕ АУДИО
================================================================ */
void audio_set_listener(AudioSystem* a,float wx,float wy,float range){
    if(!a) return;
    a->listener_x=wx; a->listener_y=wy; a->listener_range=range;
    /* Обновить все позиционные каналы */
    SDL_LockMutex(a->lock);
    for(int i=0;i<AUDIO_MAX_CHANNELS;i++){
        if(a->channels[i].active && a->channels[i].positional){
            a->channels[i].listener_x=wx;
            a->channels[i].listener_y=wy;
            a->channels[i].max_dist=range;
        }
    }
    SDL_UnlockMutex(a->lock);
}

void audio_update_channel_position(AudioSystem* a,int ci,float wx,float wy){
    if(!a||ci<0||ci>=AUDIO_MAX_CHANNELS) return;
    SDL_LockMutex(a->lock);
    a->channels[ci].world_x=wx; a->channels[ci].world_y=wy;
    SDL_UnlockMutex(a->lock);
}

/* ================================================================
   СОБЫТИЯ
================================================================ */
void audio_register_event(AudioSystem* a, const char* trigger,
                          const char* sound, float vmin, float vmax,
                          float pmin, float pmax, float cooldown){
    if(!a||a->event_count>=AUDIO_MAX_EVENTS) return;
    AudioEvent* e=&a->events[a->event_count++];
    strncpy(e->trigger_name,trigger,63);
    strncpy(e->sound_name,sound,63);
    e->volume_min=vmin; e->volume_max=vmax;
    e->pitch_min=pmin;  e->pitch_max=pmax;
    e->cooldown=cooldown;
    e->_last_played=-1e9f;
}

static float _randf(float lo,float hi){ return lo+(hi-lo)*((float)rand()/RAND_MAX); }

int audio_trigger(AudioSystem* a, const char* trigger_name){
    if(!a) return -1;
    for(int i=0;i<a->event_count;i++){
        AudioEvent* e=&a->events[i];
        if(strcmp(e->trigger_name,trigger_name)!=0) continue;
        if(a->time - e->_last_played < e->cooldown) return -1;
        e->_last_played=a->time;
        float vol=_randf(e->volume_min,e->volume_max);
        float pit=_randf(e->pitch_min,e->pitch_max);
        return audio_play(a,e->sound_name,vol,pit,e->pan);
    }
    printf("[Audio] Event '%s' not registered\n",trigger_name);
    return -1;
}

int audio_trigger_at(AudioSystem* a, const char* trigger_name, float wx, float wy){
    if(!a) return -1;
    for(int i=0;i<a->event_count;i++){
        AudioEvent* e=&a->events[i];
        if(strcmp(e->trigger_name,trigger_name)!=0) continue;
        if(a->time - e->_last_played < e->cooldown) return -1;
        e->_last_played=a->time;
        float vol=_randf(e->volume_min,e->volume_max);
        return audio_play_at(a,e->sound_name,wx,wy,vol);
    }
    return -1;
}

/* ================================================================
   УТИЛИТЫ
================================================================ */
bool  audio_is_playing(AudioSystem* a, int ci){
    if(!a||ci<0||ci>=AUDIO_MAX_CHANNELS) return false;
    return a->channels[ci].active;
}
float audio_channel_progress(AudioSystem* a, int ci){
    if(!a||ci<0||ci>=AUDIO_MAX_CHANNELS) return 0.f;
    AudioChannel* ch=&a->channels[ci];
    if(!ch->sound||!ch->active) return 0.f;
    return (float)ch->position/(float)ch->sound->length;
}
int audio_active_channels(AudioSystem* a){
    if(!a) return 0;
    int n=0;
    for(int i=0;i<AUDIO_MAX_CHANNELS;i++) if(a->channels[i].active) n++;
    return n;
}
Sound* audio_find_sound(AudioSystem* a, const char* name){
    if(!a) return NULL;
    int i=_find_sound(a,name);
    return (i>=0)?&a->sounds[i]:NULL;
}
