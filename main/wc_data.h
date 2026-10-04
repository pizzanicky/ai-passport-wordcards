#pragma once
#include <stdint.h>
#define WC_WORD_COUNT 3676
#define WC_CORPUS_ID 1156457013u
typedef struct {const char *word,*ipa,*definition;uint32_t id,audio_offset,audio_length;} wc_word_t;
extern const wc_word_t wc_words[WC_WORD_COUNT];
extern const uint32_t wc_word_ids[WC_WORD_COUNT];
