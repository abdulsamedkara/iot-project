#pragma once
#include "lvgl.h"
#include "esp_err.h"

typedef enum {
    SCREEN_IDLE,
    SCREEN_POMODORO,
    SCREEN_SENSOR,
    SCREEN_VOICE,
    SCREEN_AI
} screen_id_t;

extern lv_obj_t *scr_idle;
extern lv_obj_t *scr_pomodoro;
extern lv_obj_t *scr_sensor;
extern lv_obj_t *scr_voice;
extern lv_obj_t *scr_ai;

esp_err_t display_init(void);
void display_switch_screen(screen_id_t id);
bool display_lock(int timeout_ms);
void display_unlock(void);
