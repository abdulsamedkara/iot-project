#include "ui_idle.h"
#include "../lang.h"
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

static lv_obj_t *lbl_clock;
static lv_obj_t *lbl_date;
static lv_obj_t *lbl_status;
static lv_obj_t *bar_breath;
static uint32_t s_fake_hh = 9, s_fake_mm = 0, s_fake_ss = 0;

// Nefes animasyonu için callback
static void breath_anim_cb(void *var, int32_t v) {
    lv_obj_t *bar = (lv_obj_t *)var;
    lv_bar_set_value(bar, v, LV_ANIM_OFF);
}

void ui_idle_init(lv_obj_t *parent) {
    // ─── Arka Plan ───────────────────────────────────────────
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0D1B2A), 0);
    lv_obj_set_style_text_color(parent, lv_color_white(), 0);

    // ─── Sol Üst Köşe: Cihaz İsmi ────────────────────────────
    lv_obj_t *lbl_name = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl_name, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_name, lv_color_hex(0x546E7A), 0);
    lv_label_set_text(lbl_name, "POMODORO BOT");
    lv_obj_align(lbl_name, LV_ALIGN_TOP_LEFT, 10, 8);

    // ─── Sağ Üst Köşe: WiFi İkonu ────────────────────────────
    lv_obj_t *lbl_wifi = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl_wifi, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_wifi, lv_color_hex(0x4CAF50), 0);
    lv_label_set_text(lbl_wifi, LV_SYMBOL_WIFI);
    lv_obj_align(lbl_wifi, LV_ALIGN_TOP_RIGHT, -10, 8);

    // ─── Yatay Ayırıcı Çizgi (üst) ────────────────────────────
    lv_obj_t *sep_top = lv_obj_create(parent);
    lv_obj_set_size(sep_top, 220, 1);
    lv_obj_align(sep_top, LV_ALIGN_TOP_MID, 0, 28);
    lv_obj_set_style_bg_color(sep_top, lv_color_hex(0x1E3040), 0);
    lv_obj_set_style_border_width(sep_top, 0, 0);

    // ─── Büyük Saat ──────────────────────────────────────────
    lbl_clock = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl_clock, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(lbl_clock, lv_color_white(), 0);
    lv_label_set_text(lbl_clock, "09:00");
    lv_obj_align(lbl_clock, LV_ALIGN_CENTER, 0, -22);

    // ─── Tarih ───────────────────────────────────────────────
    // montserrat_20 + saf beyaz → kesinlikle okunabilir
    lbl_date = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl_date, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_date, lv_color_white(), 0);
    lv_label_set_text(lbl_date, "21.03.2026");
    lv_obj_align(lbl_date, LV_ALIGN_CENTER, 0, 14);

    // ─── Yatay Ayırıcı Çizgi ─────────────────────────────────
    lv_obj_t *sep_bot = lv_obj_create(parent);
    lv_obj_set_size(sep_bot, 140, 1);
    lv_obj_align(sep_bot, LV_ALIGN_CENTER, 0, 38);
    lv_obj_set_style_bg_color(sep_bot, lv_color_hex(0x1E3040), 0);
    lv_obj_set_style_border_width(sep_bot, 0, 0);

    // ─── Durum Metni (HAZIR) ─────────────────────────────────
    lbl_status = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_status, lv_color_hex(0x4CAF50), 0);
    lv_label_set_text(lbl_status, STR_STATUS_READY);
    lv_obj_align(lbl_status, LV_ALIGN_CENTER, 0, 52);

    // ─── Nefes Çubuğu ────────────────────────────────────────
    bar_breath = lv_bar_create(parent);
    lv_obj_set_size(bar_breath, 140, 4);
    lv_bar_set_range(bar_breath, 0, 100);
    lv_bar_set_value(bar_breath, 30, LV_ANIM_OFF);
    lv_obj_align(bar_breath, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(bar_breath, lv_color_hex(0x12263A), 0);
    lv_obj_set_style_radius(bar_breath, 2, 0);
    lv_obj_set_style_bg_color(bar_breath, lv_color_hex(0x29B6F6), LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_breath, 2, LV_PART_INDICATOR);

    // Nefes animasyonu
    static lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, bar_breath);
    lv_anim_set_values(&a, 15, 85);
    lv_anim_set_time(&a, 2000);
    lv_anim_set_playback_time(&a, 2000);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&a, breath_anim_cb);
    lv_anim_start(&a);
}

void ui_idle_tick(void) {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    char buf[16];
    if (timeinfo.tm_year > (2024 - 1900)) {
        snprintf(buf, sizeof(buf), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
        lv_label_set_text(lbl_clock, buf);
        
        char buf_date[32];
        snprintf(buf_date, sizeof(buf_date), "%02d.%02d.%04d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
        lv_label_set_text(lbl_date, buf_date);
    } else {
        s_fake_ss++;
        if (s_fake_ss >= 60) { s_fake_ss = 0; s_fake_mm++; }
        if (s_fake_mm >= 60) { s_fake_mm = 0; s_fake_hh++; }
        if (s_fake_hh >= 24) s_fake_hh = 0;

        snprintf(buf, sizeof(buf), "%02lu:%02lu", (unsigned long)s_fake_hh, (unsigned long)s_fake_mm);
        lv_label_set_text(lbl_clock, buf);
    }
}
