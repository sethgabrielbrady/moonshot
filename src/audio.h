#ifndef AUDIO_H
#define AUDIO_H

#include <libdragon.h>

extern wav64_t sfx_mining;
extern wav64_t sfx_dcom;
extern wav64_t sfx_dfull;
extern wav64_t sfx_shiphit;
extern wav64_t bgm;

extern bool bgm_playing;

// void init_audio();
void play_sfx( int sfx_type );
void play_bgm(const char *filename);
void stop_bgm();
void update_audio();
void set_bgm_volume(float volume);
// extern stop_audio();


#endif //AUDIO_H