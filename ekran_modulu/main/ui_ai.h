#pragma once
#include "lvgl.h"

void ui_ai_init(lv_obj_t *parent);
void ui_ai_set_response(const char *text, const char *source);
void ui_ai_set_thinking(bool thinking);
