#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
esp_err_t wc_audio_init(void);
bool wc_audio_play(uint16_t word,uint8_t volume);
bool wc_audio_relearn(uint16_t word,uint8_t volume);
bool wc_audio_feedback(bool familiar,uint8_t volume);
bool wc_audio_stop(void);
esp_err_t wc_audio_take_error(void);
