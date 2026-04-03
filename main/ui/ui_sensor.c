#include "ui_sensor.h"
#include "../lang.h"
#include <stdio.h>

static lv_obj_t *lbl_temp_val;
static lv_obj_t *lbl_temp_state;
static lv_obj_t *bar_light;
static lv_obj_t *lbl_light_pct;
static lv_obj_t *lbl_light_state;
static lv_obj_t *bar_comfort;
static lv_obj_t *lbl_comfort_pct;
static lv_obj_t *icon_warning;
static lv_obj_t *lbl_noise_val;
static lv_obj_t *bar_noise;

void ui_sensor_init(lv_obj_t *parent) {
    // ─── Background ──────────────────────────────────────────
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_text_color(parent, lv_color_white(), 0);

    // ─── Title ───────────────────────────────────────────────
    lv_obj_t *lbl_title = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0x78909C), 0);
    lv_label_set_text(lbl_title, STR_ENV_ANALYSIS);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 8);

    // ─── Warning Icon (hidden by default) ────────────────────
    icon_warning = lv_label_create(parent);
    lv_obj_set_style_text_font(icon_warning, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(icon_warning, lv_color_hex(0xFF5252), 0);
    lv_label_set_text(icon_warning, LV_SYMBOL_WARNING);
    lv_obj_align(icon_warning, LV_ALIGN_TOP_RIGHT, -8, 8);
    lv_obj_add_flag(icon_warning, LV_OBJ_FLAG_HIDDEN);

    // ─── Top Separator ───────────────────────────────────────
    lv_obj_t *sep_top = lv_obj_create(parent);
    lv_obj_set_size(sep_top, 224, 1);
    lv_obj_align(sep_top, LV_ALIGN_TOP_MID, 0, 28);
    lv_obj_set_style_bg_color(sep_top, lv_color_hex(0x1E3040), 0);
    lv_obj_set_style_border_width(sep_top, 0, 0);

    // =========================================================
    // CARD 1 — TEMPERATURE (Left half)
    // =========================================================
    lv_obj_t *card_temp = lv_obj_create(parent);
    lv_obj_set_size(card_temp, 112, 95);
    lv_obj_align(card_temp, LV_ALIGN_TOP_LEFT, 2, 31);
    lv_obj_set_style_bg_color(card_temp, lv_color_hex(0x0E2233), 0);
    lv_obj_set_style_border_color(card_temp, lv_color_hex(0x1B3A4B), 0);
    lv_obj_set_style_border_width(card_temp, 1, 0);
    lv_obj_set_style_radius(card_temp, 6, 0);
    lv_obj_set_style_pad_all(card_temp, 6, 0);
    lv_obj_clear_flag(card_temp, LV_OBJ_FLAG_SCROLLABLE);

    // Card title row: icon + label
    lv_obj_t *lbl_temp_icon = lv_label_create(card_temp);
    lv_obj_set_style_text_font(lbl_temp_icon, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_temp_icon, lv_color_hex(0xFF7043), 0);
    lv_label_set_text(lbl_temp_icon, LV_SYMBOL_TINT);
    lv_obj_align(lbl_temp_icon, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *lbl_temp_title = lv_label_create(card_temp);
    lv_obj_set_style_text_font(lbl_temp_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_temp_title, lv_color_hex(0xB0BEC5), 0);
    lv_label_set_text(lbl_temp_title, STR_TEMP_TITLE);
    lv_obj_align_to(lbl_temp_title, lbl_temp_icon, LV_ALIGN_OUT_RIGHT_MID, 3, 0);

    // Temperature value — large, bold
    lbl_temp_val = lv_label_create(card_temp);
    lv_obj_set_style_text_font(lbl_temp_val, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_temp_val, lv_color_white(), 0);
    lv_label_set_text(lbl_temp_val, "--°C");
    lv_obj_align(lbl_temp_val, LV_ALIGN_CENTER, 0, 4);

    // Status label — below card
    lbl_temp_state = lv_label_create(card_temp);
    lv_obj_set_style_text_font(lbl_temp_state, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_temp_state, lv_color_hex(0x4CAF50), 0);
    lv_label_set_text(lbl_temp_state, "OK");
    lv_obj_align(lbl_temp_state, LV_ALIGN_BOTTOM_MID, 0, 0);

    // =========================================================
    // CARD 2 — LIGHT INTENSITY (Right half)
    // =========================================================
    lv_obj_t *card_light = lv_obj_create(parent);
    lv_obj_set_size(card_light, 112, 95);
    lv_obj_align(card_light, LV_ALIGN_TOP_RIGHT, -2, 31);
    lv_obj_set_style_bg_color(card_light, lv_color_hex(0x0E1F33), 0);
    lv_obj_set_style_border_color(card_light, lv_color_hex(0x1B3A4B), 0);
    lv_obj_set_style_border_width(card_light, 1, 0);
    lv_obj_set_style_radius(card_light, 6, 0);
    lv_obj_set_style_pad_all(card_light, 6, 0);
    lv_obj_clear_flag(card_light, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_light_icon = lv_label_create(card_light);
    lv_obj_set_style_text_font(lbl_light_icon, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_light_icon, lv_color_hex(0xFFEE58), 0);
    lv_label_set_text(lbl_light_icon, LV_SYMBOL_EYE_OPEN);
    lv_obj_align(lbl_light_icon, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *lbl_light_title = lv_label_create(card_light);
    lv_obj_set_style_text_font(lbl_light_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_light_title, lv_color_hex(0xB0BEC5), 0);
    lv_label_set_text(lbl_light_title, STR_LIGHT_TITLE);
    lv_obj_align_to(lbl_light_title, lbl_light_icon, LV_ALIGN_OUT_RIGHT_MID, 3, 0);

    // Percentage value — large
    lbl_light_pct = lv_label_create(card_light);
    lv_obj_set_style_text_font(lbl_light_pct, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_light_pct, lv_color_white(), 0);
    lv_label_set_text(lbl_light_pct, "--%");
    lv_obj_align(lbl_light_pct, LV_ALIGN_CENTER, 0, -2);

    // Bar — thin, at bottom
    bar_light = lv_bar_create(card_light);
    lv_obj_set_size(bar_light, 88, 6);
    lv_obj_align(bar_light, LV_ALIGN_CENTER, 0, 16);
    lv_bar_set_range(bar_light, 0, 100);
    lv_obj_set_style_bg_color(bar_light, lv_color_hex(0x1B3A4B), 0);
    lv_obj_set_style_radius(bar_light, 3, 0);
    lv_obj_set_style_bg_color(bar_light, lv_color_hex(0xFFEE58), LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_light, 3, LV_PART_INDICATOR);

    // Status label
    lbl_light_state = lv_label_create(card_light);
    lv_obj_set_style_text_font(lbl_light_state, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_light_state, lv_color_hex(0x4CAF50), 0);
    lv_label_set_text(lbl_light_state, STR_GOOD);
    lv_obj_align(lbl_light_state, LV_ALIGN_BOTTOM_MID, 0, 0);

    // =========================================================
    // CARD 3 — NOISE (Middle)
    // =========================================================
    lv_obj_t *card_noise = lv_obj_create(parent);
    lv_obj_set_size(card_noise, 224, 60);
    lv_obj_align(card_noise, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_bg_color(card_noise, lv_color_hex(0x0E233A), 0);
    lv_obj_set_style_border_color(card_noise, lv_color_hex(0x1B3A4B), 0);
    lv_obj_set_style_border_width(card_noise, 1, 0);
    lv_obj_set_style_radius(card_noise, 6, 0);
    lv_obj_set_style_pad_all(card_noise, 6, 0);
    lv_obj_clear_flag(card_noise, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_noise_icon = lv_label_create(card_noise);
    lv_obj_set_style_text_font(lbl_noise_icon, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_noise_icon, lv_color_hex(0x9FA8DA), 0);
    lv_label_set_text(lbl_noise_icon, LV_SYMBOL_VOLUME_MAX);
    lv_obj_align(lbl_noise_icon, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *lbl_noise_title = lv_label_create(card_noise);
    lv_obj_set_style_text_font(lbl_noise_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_noise_title, lv_color_hex(0xB0BEC5), 0);
    lv_label_set_text(lbl_noise_title, STR_NOISE_TITLE);
    lv_obj_align_to(lbl_noise_title, lbl_noise_icon, LV_ALIGN_OUT_RIGHT_MID, 3, 0);

    lbl_noise_val = lv_label_create(card_noise);
    lv_obj_set_style_text_font(lbl_noise_val, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_noise_val, lv_color_white(), 0);
    lv_label_set_text(lbl_noise_val, "-- dB");
    lv_obj_align(lbl_noise_val, LV_ALIGN_TOP_RIGHT, -4, -2);

    bar_noise = lv_bar_create(card_noise);
    lv_obj_set_size(bar_noise, 200, 8);
    lv_obj_align(bar_noise, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_bar_set_range(bar_noise, 0, 100);
    lv_obj_set_style_bg_color(bar_noise, lv_color_hex(0x1B3A4B), 0);
    lv_obj_set_style_radius(bar_noise, 4, 0);
    lv_obj_set_style_bg_color(bar_noise, lv_color_hex(0x9FA8DA), LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_noise, 4, LV_PART_INDICATOR);

    // =========================================================
    // BOTTOM CARD — COMFORT SCORE
    // =========================================================
    lv_obj_t *card_comfort = lv_obj_create(parent);
    lv_obj_set_size(card_comfort, 224, 50);
    lv_obj_align(card_comfort, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(card_comfort, lv_color_hex(0x0E233A), 0);
    lv_obj_set_style_border_color(card_comfort, lv_color_hex(0x1B3A4B), 0);
    lv_obj_set_style_border_width(card_comfort, 1, 0);
    lv_obj_set_style_border_side(card_comfort, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_radius(card_comfort, 0, 0);
    lv_obj_set_style_pad_hor(card_comfort, 10, 0);
    lv_obj_set_style_pad_ver(card_comfort, 6, 0);
    lv_obj_clear_flag(card_comfort, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_comfort_title = lv_label_create(card_comfort);
    lv_obj_set_style_text_font(lbl_comfort_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_comfort_title, lv_color_hex(0x90A4AE), 0);
    lv_label_set_text(lbl_comfort_title, STR_COMFORT_TITLE);
    lv_obj_align(lbl_comfort_title, LV_ALIGN_TOP_LEFT, 0, 0);

    lbl_comfort_pct = lv_label_create(card_comfort);
    lv_obj_set_style_text_font(lbl_comfort_pct, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_comfort_pct, lv_color_white(), 0);
    lv_label_set_text(lbl_comfort_pct, "--%");
    lv_obj_align(lbl_comfort_pct, LV_ALIGN_TOP_RIGHT, 0, -2);

    bar_comfort = lv_bar_create(card_comfort);
    lv_obj_set_size(bar_comfort, 196, 8);
    lv_obj_align(bar_comfort, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_bar_set_range(bar_comfort, 0, 100);
    lv_obj_set_style_bg_color(bar_comfort, lv_color_hex(0x1B3A4B), 0);
    lv_obj_set_style_radius(bar_comfort, 4, 0);
    lv_obj_set_style_bg_color(bar_comfort, lv_color_hex(0x4CAF50), LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_comfort, 4, LV_PART_INDICATOR);
}

void ui_sensor_update(float temp_c, int ldr_value, float noise_db) {
    // ─── Update Temperature ──────────────────────────────────
    char buf[20];
    snprintf(buf, sizeof(buf), "%.1f°C", temp_c);
    lv_label_set_text(lbl_temp_val, buf);

    lv_color_t temp_color;
    const char *temp_str;
    int temp_score;
    if (temp_c < 18.0) {
        temp_color = lv_color_hex(0x29B6F6); temp_str = STR_COLD; temp_score = 40;
    } else if (temp_c > 28.0) {
        temp_color = lv_color_hex(0xFF5252); temp_str = STR_HOT; temp_score = 30;
    } else {
        temp_color = lv_color_hex(0x4CAF50); temp_str = STR_COMFORT_OK; temp_score = 100;
    }
    lv_obj_set_style_text_color(lbl_temp_state, temp_color, 0);
    lv_label_set_text(lbl_temp_state, temp_str);

    // ─── Update Light ────────────────────────────────────────
    int light_pct = ldr_value; // main.c already sends percentage between 0-100
    if (light_pct > 100) light_pct = 100;
    lv_bar_set_value(bar_light, light_pct, LV_ANIM_ON);
    snprintf(buf, sizeof(buf), "%d%%", light_pct);
    lv_label_set_text(lbl_light_pct, buf);

    int light_score;
    const char *light_str;
    lv_color_t light_color;
    if (light_pct < 15) {
        light_str = STR_DARK; light_color = lv_color_hex(0xFF5252); light_score = 20;
    } else if (light_pct < 30) {
        light_str = STR_DIM; light_color = lv_color_hex(0xFFA726); light_score = 60;
    } else {
        light_str = STR_GOOD; light_color = lv_color_hex(0x4CAF50); light_score = 100;
    }
    lv_obj_set_style_text_color(lbl_light_state, light_color, 0);
    lv_label_set_text(lbl_light_state, light_str);

    // ─── Update Noise ────────────────────────────────────────
    snprintf(buf, sizeof(buf), "%.1f dB", noise_db);
    lv_label_set_text(lbl_noise_val, buf);
    int noise_pct = (int)noise_db;
    if (noise_pct > 100) noise_pct = 100;
    lv_bar_set_value(bar_noise, noise_pct, LV_ANIM_ON);
    lv_color_t noise_color = (noise_pct > 50) ? lv_color_hex(0xFF5252) : lv_color_hex(0x4CAF50);
    lv_obj_set_style_bg_color(bar_noise, noise_color, LV_PART_INDICATOR);

    // ─── Comfort Score (Avg of Temp + Light + Noise) ─────────
    int noise_score;
    if (noise_pct < 30)       noise_score = 100; // quiet
    else if (noise_pct < 55)  noise_score = 60;  // normal
    else                       noise_score = 20;  // noisy

    int comfort = (temp_score + light_score + noise_score) / 3;
    lv_bar_set_value(bar_comfort, comfort, LV_ANIM_ON);
    snprintf(buf, sizeof(buf), "%d%%", comfort);
    lv_label_set_text(lbl_comfort_pct, buf);
    lv_color_t comfort_color = (comfort >= 70) ? lv_color_hex(0x4CAF50) :
                               (comfort >= 40) ? lv_color_hex(0xFFA726) : lv_color_hex(0xFF5252);
    lv_obj_set_style_bg_color(bar_comfort, comfort_color, LV_PART_INDICATOR);
    lv_obj_set_style_text_color(lbl_comfort_pct, comfort_color, 0);

    // ─── Warning Icon ────────────────────────────────────────
    if (comfort < 50) {
        lv_obj_clear_flag(icon_warning, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(icon_warning, LV_OBJ_FLAG_HIDDEN);
    }
}
