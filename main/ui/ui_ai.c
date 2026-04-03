#include "ui_ai.h"
#include "../lang.h"

static lv_obj_t *lbl_thinking;
static lv_obj_t *lbl_response;
static lv_obj_t *lbl_source;
static lv_obj_t *lbl_question;
static lv_anim_t anim_think;

static void think_anim_cb(void *var, int32_t v) {
    lv_obj_set_style_text_opa((lv_obj_t*)var, (lv_opa_t)v, 0);
}

void ui_ai_init(lv_obj_t *parent) {
    // ─── Background ──────────────────────────────────────────
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x0A0E1A), 0);
    lv_obj_set_style_text_color(parent, lv_color_white(), 0);

    // ─── Title: icon + "AI" ──────────────────────────────────
    lv_obj_t *lbl_icon = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl_icon, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_icon, lv_color_hex(0x7C83FD), 0);
    lv_label_set_text(lbl_icon, LV_SYMBOL_LOOP);
    lv_obj_align(lbl_icon, LV_ALIGN_TOP_LEFT, 8, 8);

    lv_obj_t *lbl_title = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_16, 0);
    // Bright near-white color — clear on screen
    lv_obj_set_style_text_color(lbl_title, lv_color_white(), 0);
    lv_label_set_text(lbl_title, STR_AI_TITLE);
    lv_obj_align_to(lbl_title, lbl_icon, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

    // ─── Top Separator ───────────────────────────────────────
    lv_obj_t *sep = lv_obj_create(parent);
    lv_obj_set_size(sep, 224, 1);
    lv_obj_align(sep, LV_ALIGN_TOP_MID, 0, 28);
    lv_obj_set_style_bg_color(sep, lv_color_hex(0x1E2340), 0);
    lv_obj_set_style_border_width(sep, 0, 0);

    // ─── QUESTION BOX ────────────────────────────────────────
    lv_obj_t *cont_q = lv_obj_create(parent);
    lv_obj_set_size(cont_q, 224, 56);
    lv_obj_align(cont_q, LV_ALIGN_TOP_MID, 0, 32);
    lv_obj_set_style_bg_color(cont_q, lv_color_hex(0x0D1B3E), 0);
    lv_obj_set_style_border_color(cont_q, lv_color_hex(0x3949AB), 0);
    lv_obj_set_style_border_width(cont_q, 1, 0);
    lv_obj_set_style_radius(cont_q, 5, 0);
    lv_obj_set_style_pad_hor(cont_q, 6, 0);
    lv_obj_set_style_pad_ver(cont_q, 4, 0);
    lv_obj_clear_flag(cont_q, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_q_title = lv_label_create(cont_q);
    lv_obj_set_style_text_font(lbl_q_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_q_title, lv_color_hex(0x7986CB), 0);
    lv_label_set_text(lbl_q_title, STR_QUESTION);
    lv_obj_align(lbl_q_title, LV_ALIGN_TOP_LEFT, 0, 0);

    lbl_question = lv_label_create(cont_q);
    lv_label_set_long_mode(lbl_question, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl_question, 208);
    lv_obj_set_style_text_font(lbl_question, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_question, lv_color_hex(0xCDD7FF), 0);
    lv_label_set_text(lbl_question, "...");
    lv_obj_align(lbl_question, LV_ALIGN_TOP_LEFT, 0, 18);

    // ─── ANSWER BOX ──────────────────────────────────────────
    lv_obj_t *cont_resp = lv_obj_create(parent);
    lv_obj_set_size(cont_resp, 224, 200);
    lv_obj_align(cont_resp, LV_ALIGN_TOP_MID, 0, 92);
    lv_obj_set_style_bg_color(cont_resp, lv_color_hex(0x0D1226), 0);
    lv_obj_set_style_border_color(cont_resp, lv_color_hex(0x1E2340), 0);
    lv_obj_set_style_border_width(cont_resp, 1, 0);
    lv_obj_set_style_radius(cont_resp, 5, 0);
    lv_obj_set_style_pad_hor(cont_resp, 6, 0);
    lv_obj_set_style_pad_ver(cont_resp, 4, 0);
    // Keep the scroll active (so text can flow down)
    // lv_obj_clear_flag(cont_resp, LV_OBJ_FLAG_SCROLLABLE);

    // Source label (who provided the answer)
    lbl_source = lv_label_create(cont_resp);
    lv_obj_set_style_text_font(lbl_source, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_source, lv_color_hex(0x7986CB), 0);
    lv_label_set_text(lbl_source, STR_ANSWER_SRC);
    lv_obj_align(lbl_source, LV_ALIGN_TOP_LEFT, 0, 0);

    lbl_response = lv_label_create(cont_resp);
    lv_label_set_long_mode(lbl_response, LV_LABEL_LONG_WRAP); // Words wrap to next line
    lv_obj_set_width(lbl_response, 208);
    lv_obj_set_style_text_font(lbl_response, &lv_font_montserrat_14, 0);
    // Bright whitish color
    lv_obj_set_style_text_color(lbl_response, lv_color_white(), 0);
    lv_label_set_text(lbl_response, STR_ANSWER_WAIT);
    lv_obj_align(lbl_response, LV_ALIGN_TOP_LEFT, 0, 18);

    // ─── Bottom: "thinking..." + Accent Line ─────────────────
    // Unanimated — only show/hide in thinking state
    lbl_thinking = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl_thinking, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_thinking, lv_color_hex(0x9FA8DA), 0);
    lv_label_set_text(lbl_thinking, STR_THINKING);
    lv_obj_align(lbl_thinking, LV_ALIGN_BOTTOM_LEFT, 8, -6);
    lv_obj_add_flag(lbl_thinking, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *accent = lv_obj_create(parent);
    lv_obj_set_size(accent, 224, 3);
    lv_obj_align(accent, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(accent, lv_color_hex(0x3949AB), 0);
    lv_obj_set_style_border_width(accent, 0, 0);
    lv_obj_set_style_radius(accent, 0, 0);

    // ─── Thinking Animation ──────────────────────────────────
    lv_anim_init(&anim_think);
    lv_anim_set_var(&anim_think, lbl_thinking);
    lv_anim_set_values(&anim_think, 100, 255);   // min 100 -> doesn't disappear completely
    lv_anim_set_time(&anim_think, 1000);          // slower
    lv_anim_set_playback_time(&anim_think, 1000);
    lv_anim_set_repeat_count(&anim_think, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&anim_think, think_anim_cb);
}

void ui_ai_set_response(const char *text, const char *source) {
    lv_label_set_text(lbl_response, text);
    if (source) {
        // source now holds the question (source label removed, writing to lbl_question)
        lv_label_set_text(lbl_question, source);
    }
}

void ui_ai_set_thinking(bool thinking) {
    if (thinking) {
        lv_obj_clear_flag(lbl_thinking, LV_OBJ_FLAG_HIDDEN);
        lv_anim_start(&anim_think);
    } else {
        lv_anim_del(lbl_thinking, think_anim_cb);
        lv_obj_set_style_text_opa(lbl_thinking, 255, 0);
        lv_obj_add_flag(lbl_thinking, LV_OBJ_FLAG_HIDDEN);
    }
}
