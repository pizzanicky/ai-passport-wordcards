#pragma once
#include <stdint.h>
typedef struct {const int16_t *pcm;uint32_t count;} wc_feedback_t;
extern const wc_feedback_t wc_feedback_known,wc_feedback_again;
