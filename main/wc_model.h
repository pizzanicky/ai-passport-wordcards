#pragma once
#include <stdint.h>
#include <stdbool.h>
#define WC_MAX_WORDS 4096
#define WC_ROUND_LIMIT 20
#define WC_NEW_LIMIT 5
typedef struct { uint32_t id, due; uint8_t stage, seen; uint16_t reserved; } wc_progress_t;
typedef struct {
 uint32_t round, corpus;
 uint16_t count, queue[WC_ROUND_LIMIT];
 uint8_t length, cursor, recognized, again, volume, revealed, reserved[2];
 wc_progress_t words[WC_MAX_WORDS];
} wc_state_t;
void wc_init(wc_state_t *s, const uint32_t *ids, uint16_t n, uint32_t corpus);
void wc_upgrade_defaults(wc_state_t *s);
bool wc_valid(const wc_state_t *s);
bool wc_start(wc_state_t *s);
bool wc_begin_relearn(wc_state_t *s);
bool wc_rate(wc_state_t *s, bool familiar);
uint16_t wc_due_count(const wc_state_t *s);
uint16_t wc_seen_count(const wc_state_t *s);
uint16_t wc_familiar_count(const wc_state_t *s);
uint32_t wc_crc32(const void *data, unsigned size);
bool wc_migrate(wc_state_t *s, const wc_state_t *old);
