#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"
#include "esp_audio_types.h"
esp_err_t wc_decoder_setup(void);
esp_err_t wc_decoder_error(esp_audio_err_t result);
