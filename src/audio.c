#include "audio.h"


wav64_t sfx_mining;
wav64_t sfx_dcom;
wav64_t sfx_dfull;
wav64_t sfx_shiphit;
wav64_t bgm;
bool bgm_playing = false;

void play_sfx(int sfx_type) {
    // Use switch to play only the requested sound effect
    // Note: Stereo sounds use 2 consecutive channels, so we skip by 2
    switch (sfx_type) {
        case 1:  // Mining sound (uses channels 2-3)
            if (!mixer_ch_playing(2)) {
                wav64_play(&sfx_mining, 2);
                mixer_ch_set_vol(2, 0.3, 0.3);
            }
            break;

        case 2:  // Drone collecting (uses channels 4-5)
            if (!mixer_ch_playing(4)) {
                wav64_play(&sfx_dcom, 4);
                mixer_ch_set_vol(4, 0.3, 0.3);
            }
            break;

        case 3:  // Drone full (uses channels 6-7)
            if (!mixer_ch_playing(6)) {
                wav64_play(&sfx_dfull, 6);
                mixer_ch_set_vol(6, 0.3, 0.3);
                mixer_ch_set_freq(6, 1040.0f);
            }
            break;

        case 4:  // Ship hit (uses channels 8-9)
            // Stop and restart if already playing
            if (mixer_ch_playing(8)) {
                mixer_ch_stop(8);
            }
            wav64_play(&sfx_shiphit, 8);
            mixer_ch_set_vol(8, 0.5, 0.5);
            mixer_ch_set_freq(8, 840.0f);
            break;
        case 5:  // Ship hit (uses channels 8-9)
            // Stop and restart if already playing
            if (mixer_ch_playing(10)) {
                mixer_ch_stop(10);
            }
            wav64_play(&sfx_shiphit, 10);
            mixer_ch_set_vol(10, 0.9, 0.9);
            mixer_ch_set_freq(10, 440.0f);
            break;
    }
}


// Load and play background music
void play_bgm(const char *filename) {
    if (bgm_playing) return;  // Already playing

    wav64_open(&bgm, filename);
    wav64_set_loop(&bgm, true);  // Loop forever
    wav64_play(&bgm, 0);         // Play on channel 0
    bgm_playing = true;
}

// Stop background music
void stop_bgm(void) {
    if (!bgm_playing) return;

    mixer_ch_stop(0);
    wav64_close(&bgm);
    bgm_playing = false;
}

void set_bgm_volume(float volume) {
    // volume: 0.0 to 1.0
    mixer_ch_set_vol(0, volume, volume);  // Left, Right
}

// Call every frame to keep audio playing
void update_audio() {
    if (audio_can_write()) {
        short *buf = audio_write_begin();
        mixer_poll(buf, audio_get_buffer_length());
        audio_write_end();
    }
}