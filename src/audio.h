#ifndef AUDIO_H
#define AUDIO_H

#include <libdragon.h>

// =============================================================================
// Sound Effect Handles
// =============================================================================

extern wav64_t sfx_mining;
extern wav64_t sfx_dcom;
extern wav64_t sfx_dfull;
extern wav64_t sfx_shiphit;
extern wav64_t bgm;

extern bool bgm_playing;

// =============================================================================
// Sound Effect IDs
// =============================================================================

#define SFX_MINING      1
#define SFX_DRONE_CMD   2
#define SFX_DRONE_FULL  3
#define SFX_SHIP_HIT    4
#define SFX_EXPLOSION   5

// =============================================================================
// Functions
// =============================================================================

void play_sfx(int sfx_type);
void play_bgm(const char *filename);
void stop_bgm(void);
void set_bgm_volume(float volume);
void update_audio(void);

#endif // AUDIO_H
