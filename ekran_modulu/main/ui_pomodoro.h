#pragma once
#include "lvgl.h"

typedef enum {
    POMO_IDLE,
    POMO_WORK,
    POMO_BREAK
} pomo_state_t;

void ui_pomodoro_init(lv_obj_t *parent);
void ui_pomodoro_update(pomo_state_t state, uint32_t remaining_sec, uint32_t total_sec, int completed_count);
