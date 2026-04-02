#pragma once
#include "lvgl.h"

void ui_idle_init(lv_obj_t *parent);
void ui_idle_tick(void); // Her saniye çağrılır, sayacı ilerletir
