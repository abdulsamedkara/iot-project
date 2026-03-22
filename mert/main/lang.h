#pragma once

// ─── Dil Seçimi ───────────────────────────────────────────────────────────────
// Yalnızca bir tanesi aktif olmalı.
// Only one of these should be active at a time.
// #define LANG_TR
#define LANG_EN

#if defined(LANG_TR) && defined(LANG_EN)
#  error "Hem LANG_TR hem LANG_EN tanımlanamaz. Sadece birini etkin bırakın."
#endif
#if !defined(LANG_TR) && !defined(LANG_EN)
#  error "LANG_TR veya LANG_EN tanımlanmalı."
#endif

// ─── UI Etiketleri ────────────────────────────────────────────────────────────

#if defined(LANG_TR)
/* --- Pomodoro UI --- */
#  define STR_READY             "HAZIR"
#  define STR_FOCUS             "ODAKLAN"
#  define STR_BREAK             "DINLEN"
#  define STR_LONG_BREAK        "UZUN MOLA"

/* --- Idle UI --- */
#  define STR_STATUS_READY      LV_SYMBOL_OK "  HAZIR"

/* --- Voice UI --- */
#  define STR_VOICE_TITLE       "SES TANIMA"
#  define STR_LISTENING         "Dinliyor..."
#  define STR_PROCESSING        "Isleniyor..."
#  define STR_WAITING           "Bekleniyor"
#  define STR_HEARD             "Duyulan:"

/* --- AI UI --- */
#  define STR_AI_TITLE          "YAPAY ZEKA"
#  define STR_QUESTION          LV_SYMBOL_AUDIO " Soru:"
#  define STR_ANSWER_SRC        LV_SYMBOL_LOOP " Cevap:"
#  define STR_ANSWER_WAIT       "Cevap bekleniyor..."
#  define STR_THINKING          "dusunuyor..."

/* --- Sensor UI --- */
#  define STR_ENV_ANALYSIS      "ORTAM ANALIZI"
#  define STR_TEMP_TITLE        "SICAKLIK"
#  define STR_LIGHT_TITLE       "ISIK"
#  define STR_NOISE_TITLE       "GURULTU"
#  define STR_COMFORT_TITLE     "KONFOR"
#  define STR_COLD              "SOGUK"
#  define STR_HOT               "SICAK"
#  define STR_COMFORT_OK        "KONFOR"
#  define STR_DARK              "KARANLIK"
#  define STR_DIM               "ZAYIF"
#  define STR_GOOD              "IYI"

#elif defined(LANG_EN)
/* --- Pomodoro UI --- */
#  define STR_READY             "READY"
#  define STR_FOCUS             "FOCUS"
#  define STR_BREAK             "BREAK"
#  define STR_LONG_BREAK        "LONG BREAK"

/* --- Idle UI --- */
#  define STR_STATUS_READY      LV_SYMBOL_OK "  READY"

/* --- Voice UI --- */
#  define STR_VOICE_TITLE       "VOICE RECOG."
#  define STR_LISTENING         "Listening..."
#  define STR_PROCESSING        "Processing..."
#  define STR_WAITING           "Waiting"
#  define STR_HEARD             "Heard:"

/* --- AI UI --- */
#  define STR_AI_TITLE          "AI ASSIST"
#  define STR_QUESTION          LV_SYMBOL_AUDIO " Question:"
#  define STR_ANSWER_SRC        LV_SYMBOL_LOOP " Answer:"
#  define STR_ANSWER_WAIT       "Waiting for response..."
#  define STR_THINKING          "thinking..."

/* --- Sensor UI --- */
#  define STR_ENV_ANALYSIS      "ENV. ANALYSIS"
#  define STR_TEMP_TITLE        "TEMPERATURE"
#  define STR_LIGHT_TITLE       "LIGHT"
#  define STR_NOISE_TITLE       "NOISE"
#  define STR_COMFORT_TITLE     "COMFORT"
#  define STR_COLD              "COLD"
#  define STR_HOT               "HOT"
#  define STR_COMFORT_OK        "COMFORT"
#  define STR_DARK              "DARK"
#  define STR_DIM               "DIM"
#  define STR_GOOD              "GOOD"

#endif /* LANG_TR / LANG_EN */
