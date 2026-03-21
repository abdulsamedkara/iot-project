#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Ekranı ve LVGL'yi başlatır
esp_err_t display_init(void);

// Ekranda gösterilen durum metnini günceller (örn: "Dinliyor...", "Düşünüyor...")
void display_update_status(const char* status_text);

// Sayacı günceller (Pomodoro fazındayken kullanılmak üzere)
void display_update_timer(int minutes, int seconds);

#ifdef __cplusplus
}
#endif
