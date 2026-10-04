#pragma once
#include "wc_model.h"
#include "esp_err.h"
// Missing or invalid slots never trigger automatic NVS erase.
esp_err_t wc_store_open(void);
esp_err_t wc_store_load(wc_state_t *state, bool *migrated);
esp_err_t wc_store_save(const wc_state_t *state);
