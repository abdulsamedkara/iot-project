#pragma once
#include "lvgl.h"

void ui_sensor_init(lv_obj_t *parent);
void ui_sensor_update(float temp_c, int ldr_value);
