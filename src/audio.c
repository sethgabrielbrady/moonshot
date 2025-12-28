#include "audio.h"


wav64_t sfx_mining;
wav64_t bgm;
bool bgm_playing = false;

void play_sfx() {
  // Audio handling
  wav64_play(&sfx_mining, 2); // Play the shot sound effect
  mixer_try_play();
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
void update_audio(void) {
    if (audio_can_write()) {
        short *buf = audio_write_begin();
        mixer_poll(buf, audio_get_buffer_length());
        audio_write_end();
    }
}