#pragma once
#include "lvgl.h"

typedef enum {
    VOICE_IDLE,
    VOICE_RECORDING,
    VOICE_PROCESSING
} voice_ui_state_t;

void ui_voice_init(lv_obj_t *parent);
void ui_voice_set_state(voice_ui_state_t state);
void ui_voice_set_transcript(const char *text);
