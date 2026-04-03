#include "ui_pomodoro.h"
#include "../lang.h"
#include <stdio.h>

static lv_obj_t *arc_time;
static lv_obj_t *lbl_mode;
static lv_obj_t *lbl_time;
static lv_obj_t *container_stars;

#define MAX_STARS 4
static lv_obj_t *stars[MAX_STARS];

void ui_pomodoro_init(lv_obj_t *parent) {
    // ─── Background ──────────────────────────────────────────
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1017), 0);

    // ─── Title ───────────────────────────────────────────────
    lv_obj_t *lbl_title = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0x90A4AE), 0);
    lv_label_set_text(lbl_title, "ESP32 POMODORO");
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 10);

    // ─── Top Separator ───────────────────────────────────────
    lv_obj_t *sep = lv_obj_create(parent);
    lv_obj_set_size(sep, 210, 1);
    lv_obj_align(sep, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_style_bg_color(sep, lv_color_hex(0x263238), 0);
    lv_obj_set_style_border_width(sep, 0, 0);

    // ─── Arc (Ring) ──────────────────────────────────────────
    arc_time = lv_arc_create(parent);
    lv_obj_set_size(arc_time, 195, 195);
    lv_arc_set_rotation(arc_time, 270);
    lv_arc_set_bg_angles(arc_time, 0, 360);
    lv_obj_set_style_arc_width(arc_time, 14, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_time, 14, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_time, lv_color_hex(0x1C2A35), LV_PART_MAIN);
    lv_obj_remove_style(arc_time, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc_time, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(arc_time, LV_ALIGN_CENTER, 0, 5);

    // ─── Mode Label (FOCUS / BREAK) ──────────────────────────
    lbl_mode = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl_mode, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_mode, lv_color_hex(0x4CAF50), 0);
    lv_label_set_recolor(lbl_mode, true);
    lv_label_set_text(lbl_mode, STR_READY);
    lv_obj_align(lbl_mode, LV_ALIGN_CENTER, 0, -18);

    // ─── Time Text ───────────────────────────────────────────
    lbl_time = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(lbl_time, lv_color_white(), 0);
    lv_label_set_text(lbl_time, "--:--");
    lv_obj_align(lbl_time, LV_ALIGN_CENTER, 0, 8);

    // ─── Pomodoro Counter (Bottom Circles) ───────────────────
    container_stars = lv_obj_create(parent);
    lv_obj_set_size(container_stars, 220, 22);
    lv_obj_align(container_stars, LV_ALIGN_BOTTOM_MID, 0, -12);
    lv_obj_set_flex_flow(container_stars, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(container_stars, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(container_stars, 0, 0);
    lv_obj_set_style_border_width(container_stars, 0, 0);
    lv_obj_set_style_pad_all(container_stars, 2, 0);
    lv_obj_set_style_pad_column(container_stars, 5, 0);

    for (int i = 0; i < MAX_STARS; i++) {
        stars[i] = lv_obj_create(container_stars);
        lv_obj_set_size(stars[i], 14, 14);
        lv_obj_set_style_radius(stars[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(stars[i], lv_color_hex(0x263238), 0);
        lv_obj_set_style_border_color(stars[i], lv_color_hex(0x4CAF50), 0);
        lv_obj_set_style_border_width(stars[i], 1, 0);
    }
}

void ui_pomodoro_update(pomo_state_t state, uint32_t remaining_sec, uint32_t total_sec, int completed_count) {
    lv_color_t mode_color;
    if (state == POMO_WORK) {
        lv_label_set_text(lbl_mode, STR_FOCUS);
        mode_color = lv_color_hex(0x4CAF50);
        lv_obj_set_style_arc_color(arc_time, mode_color, LV_PART_INDICATOR);
    } else if (state == POMO_BREAK) {
        lv_label_set_text(lbl_mode, STR_BREAK);
        mode_color = lv_color_hex(0x29B6F6);
        lv_obj_set_style_arc_color(arc_time, mode_color, LV_PART_INDICATOR);
    } else if (state == POMO_LONG_BREAK) {
        lv_label_set_text(lbl_mode, STR_LONG_BREAK);
        mode_color = lv_color_hex(0x29B6F6);
        lv_obj_set_style_arc_color(arc_time, mode_color, LV_PART_INDICATOR);
    } else {
        lv_label_set_text(lbl_mode, STR_READY);
        mode_color = lv_color_hex(0x607d8b);
        lv_obj_set_style_arc_color(arc_time, mode_color, LV_PART_INDICATOR);
    }
    lv_obj_set_style_text_color(lbl_mode, mode_color, 0);

    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d", (int)(remaining_sec / 60), (int)(remaining_sec % 60));
    lv_label_set_text(lbl_time, buf);

    if (total_sec > 0) {
        int percent = 100 - (int)((remaining_sec * 100) / total_sec);
        lv_arc_set_value(arc_time, percent);
    } else {
        lv_arc_set_value(arc_time, 0);
    }

    int display_count = completed_count % MAX_STARS;
    for (int i = 0; i < MAX_STARS; i++) {
        if (i < display_count) {
            lv_obj_set_style_bg_color(stars[i], lv_color_hex(0xFF5252), 0);  // Completed: Red
        } else {
            lv_obj_set_style_bg_color(stars[i], lv_color_hex(0x263238), 0);  // Empty: Gray
        }
    }
}
