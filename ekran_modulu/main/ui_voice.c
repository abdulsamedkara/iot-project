#include "ui_voice.h"

static lv_obj_t *lbl_mic_ring;
static lv_obj_t *lbl_mic_icon;
static lv_obj_t *lbl_status;
static lv_obj_t *lbl_transcript;
static lv_anim_t anim_ring;

static void ring_anim_cb(void *var, int32_t v) {
    lv_obj_set_style_text_opa((lv_obj_t*)var, (lv_opa_t)v, 0);
}

void ui_voice_init(lv_obj_t *parent) {
    // ─── Arka Plan ───────────────────────────────────────────
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0A0A1A), 0);
    lv_obj_set_style_text_color(parent, lv_color_white(), 0);

    // ─── Başlık ──────────────────────────────────────────────
    lv_obj_t *lbl_title = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xB0BEC5), 0);
    lv_label_set_text(lbl_title, "SES TANIMA");
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 8);

    // ─── Üst Separator ───────────────────────────────────────
    lv_obj_t *sep = lv_obj_create(parent);
    lv_obj_set_size(sep, 224, 1);
    lv_obj_align(sep, LV_ALIGN_TOP_MID, 0, 26);
    lv_obj_set_style_bg_color(sep, lv_color_hex(0x1E2840), 0);
    lv_obj_set_style_border_width(sep, 0, 0);

    // ─── Mikrofon Halka ───────────────────────────────────────
    lbl_mic_ring = lv_obj_create(parent);
    lv_obj_set_size(lbl_mic_ring, 85, 85);
    lv_obj_set_style_radius(lbl_mic_ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(lbl_mic_ring, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_border_color(lbl_mic_ring, lv_color_hex(0x607d8b), 0);
    lv_obj_set_style_border_width(lbl_mic_ring, 2, 0);
    lv_obj_align(lbl_mic_ring, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_clear_flag(lbl_mic_ring, LV_OBJ_FLAG_SCROLLABLE);

    // ─── Mikrofon İkon ────────────────────────────────────────
    lbl_mic_icon = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl_mic_icon, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(lbl_mic_icon, lv_color_hex(0x607d8b), 0);
    lv_label_set_text(lbl_mic_icon, LV_SYMBOL_AUDIO);
    lv_obj_align(lbl_mic_icon, LV_ALIGN_TOP_MID, 0, 56);

    // ─── Durum Metni ─────────────────────────────────────────
    lbl_status = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_status, lv_color_hex(0xCFD8DC), 0);
    lv_label_set_text(lbl_status, "Bekleniyor");
    lv_obj_align(lbl_status, LV_ALIGN_TOP_MID, 0, 120);

    // ─── Transkript Alanı ─────────────────────────────────────
    // Daha yüksek kutu, "Duyulan:" + metin için yeterli alan
    lv_obj_t *cont_transcript = lv_obj_create(parent);
    lv_obj_set_size(cont_transcript, 240, 54);
    lv_obj_align(cont_transcript, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(cont_transcript, lv_color_hex(0x0D1530), 0);
    lv_obj_set_style_border_color(cont_transcript, lv_color_hex(0x29B6F6), 0);
    lv_obj_set_style_border_width(cont_transcript, 1, 0);
    lv_obj_set_style_border_side(cont_transcript, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_radius(cont_transcript, 0, 0);
    lv_obj_set_style_pad_hor(cont_transcript, 8, 0);
    lv_obj_set_style_pad_ver(cont_transcript, 5, 0);
    lv_obj_clear_flag(cont_transcript, LV_OBJ_FLAG_SCROLLABLE);

    // "Duyulan:" etiketi — sarı, belirgin
    lv_obj_t *lbl_heard = lv_label_create(cont_transcript);
    lv_obj_set_style_text_font(lbl_heard, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_heard, lv_color_hex(0xFFCA28), 0);
    lv_label_set_text(lbl_heard, "Duyulan:");
    lv_obj_align(lbl_heard, LV_ALIGN_TOP_LEFT, 0, 0);

    // Transkript metni — tam beyaz, scroll ile kayan
    lbl_transcript = lv_label_create(cont_transcript);
    lv_label_set_long_mode(lbl_transcript, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(lbl_transcript, 220);
    lv_obj_set_style_text_font(lbl_transcript, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_transcript, lv_color_white(), 0);
    lv_label_set_text(lbl_transcript, "...");
    lv_obj_align(lbl_transcript, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    // ─── Halka Yanıp Sönme Animasyonu ────────────────────────
    lv_anim_init(&anim_ring);
    lv_anim_set_var(&anim_ring, lbl_mic_icon);
    lv_anim_set_values(&anim_ring, 100, 255);
    lv_anim_set_time(&anim_ring, 600);
    lv_anim_set_playback_time(&anim_ring, 600);
    lv_anim_set_repeat_count(&anim_ring, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&anim_ring, ring_anim_cb);
}

void ui_voice_set_state(voice_ui_state_t state) {
    if (state == VOICE_RECORDING) {
        lv_obj_set_style_text_color(lbl_mic_icon, lv_color_hex(0xFF5252), 0);
        lv_obj_set_style_border_color(lbl_mic_ring, lv_color_hex(0xFF5252), 0);
        lv_obj_set_style_bg_color(lbl_mic_ring, lv_color_hex(0x2E0000), 0);
        lv_label_set_text(lbl_status, "Dinliyor...");
        lv_obj_set_style_text_color(lbl_status, lv_color_hex(0xFF5252), 0);
        lv_anim_start(&anim_ring);
    } else if (state == VOICE_PROCESSING) {
        lv_anim_del(lbl_mic_icon, ring_anim_cb);
        lv_obj_set_style_text_opa(lbl_mic_icon, 255, 0);
        lv_obj_set_style_text_color(lbl_mic_icon, lv_color_hex(0x29B6F6), 0);
        lv_obj_set_style_border_color(lbl_mic_ring, lv_color_hex(0x29B6F6), 0);
        lv_obj_set_style_bg_color(lbl_mic_ring, lv_color_hex(0x00152E), 0);
        lv_label_set_text(lbl_status, "Isleniyor...");
        lv_obj_set_style_text_color(lbl_status, lv_color_hex(0x29B6F6), 0);
    } else {
        lv_anim_del(lbl_mic_icon, ring_anim_cb);
        lv_obj_set_style_text_opa(lbl_mic_icon, 255, 0);
        lv_obj_set_style_text_color(lbl_mic_icon, lv_color_hex(0x607d8b), 0);
        lv_obj_set_style_border_color(lbl_mic_ring, lv_color_hex(0x607d8b), 0);
        lv_obj_set_style_bg_color(lbl_mic_ring, lv_color_hex(0x1A1A2E), 0);
        lv_label_set_text(lbl_status, "Bekleniyor");
        lv_obj_set_style_text_color(lbl_status, lv_color_hex(0xCFD8DC), 0);
    }
}

void ui_voice_set_transcript(const char *text) {
    lv_label_set_text(lbl_transcript, text);
}
