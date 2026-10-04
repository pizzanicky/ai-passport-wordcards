#pragma once
#include "wc_model.h"
#include <stdint.h>
typedef enum {WC_HOME,WC_FRONT,WC_BACK,WC_READ,WC_FINISH,WC_PROGRESS,WC_SETTINGS,WC_VOLUME,WC_RESET,WC_ABOUT,WC_ERROR} wc_page_t;
typedef struct {wc_page_t page,return_page;uint8_t selection,read_page,read_pages;int battery;bool busy;const char *error;} wc_view_t;
void wc_ui_init(void);
void wc_ui_render(const wc_state_t *s,wc_view_t *v);
uint8_t wc_ui_definition_pages(uint16_t word);
