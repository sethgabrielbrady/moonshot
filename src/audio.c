#include "audio.h"

// =============================================================================
// Sound Effect Handles
// =============================================================================

wav64_t sfx_mining;
wav64_t sfx_dcom;
wav64_t sfx_dfull;
wav64_t sfx_shiphit;
wav64_t bgm;

bool bgm_playing = false;

// =============================================================================
// Sound Effects
// =============================================================================

void play_sfx(int sfx_type) {
    switch (sfx_type) {
        case SFX_MINING:  // Mining sound (channels 2-3)
            if (!mixer_ch_playing(2)) {
                wav64_play(&sfx_mining, 2);
                mixer_ch_set_vol(2, 0.3, 0.3);
            }
            break;

        case SFX_DRONE_CMD:  // Drone command (channels 4-5)
            if (!mixer_ch_playing(4)) {
                wav64_play(&sfx_dcom, 4);
                mixer_ch_set_vol(4, 0.3, 0.3);
            }
            break;

        case SFX_DRONE_FULL:  // Drone full (channels 6-7)
            if (!mixer_ch_playing(6)) {
                wav64_play(&sfx_dfull, 6);
                mixer_ch_set_vol(6, 0.3, 0.3);
                mixer_ch_set_freq(6, 1040.0f);
            }
            break;

        case SFX_SHIP_HIT:  // Ship hit (channels 8-9)
            if (mixer_ch_playing(8)) {
                mixer_ch_stop(8);
            }
            wav64_play(&sfx_shiphit, 8);
            mixer_ch_set_vol(8, 0.5, 0.5);
            mixer_ch_set_freq(8, 840.0f);
            break;
    }
}

// =============================================================================
// Background Music
// =============================================================================

void play_bgm(const char *filename) {
    if (bgm_playing) return;

    wav64_open(&bgm, filename);
    wav64_set_loop(&bgm, true);
    wav64_play(&bgm, 0);
    bgm_playing = true;
}

void stop_bgm(void) {
    if (!bgm_playing) return;

    mixer_ch_stop(0);
    wav64_close(&bgm);
    bgm_playing = false;
}

void set_bgm_volume(float volume) {
    mixer_ch_set_vol(0, volume, volume);
}

// =============================================================================
// Audio Update (call every frame)
// =============================================================================

void update_audio(void) {
    if (audio_can_write()) {
        short *buf = audio_write_begin();
        mixer_poll(buf, audio_get_buffer_length());
        audio_write_end();
    }
}
