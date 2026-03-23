#pragma once

/**
 * fan.h - L298N Motor Surucu ile Fan Kontrolu
 *
 * Baglanti:
 *   L298N IN4 --> ESP32 GPIO 18
 *   L298N IN3 --> GND (tek yon calisma)
 *   L298N GND --> ESP32 GND  (ZORUNLU ortak toprak)
 *
 * Kullanim:
 *   fan_init();        // Bir kez baslatma
 *   fan_on();          // Fani ac
 *   fan_off();         // Fani kapat
 *   fan_toggle();      // Durumunu tersine cevir
 *   fan_is_on();       // Mevcut durumu oku
 */

#include <stdbool.h>

/**
 * @brief GPIO'yu cikis olarak yapilandirir ve fani kapali konumda baslatir.
 */
void fan_init(void);

/**
 * @brief Fani acar (IN4 = HIGH).
 */
void fan_on(void);

/**
 * @brief Fani kapatir (IN4 = LOW).
 */
void fan_off(void);

/**
 * @brief Fan aciksa kapatir, kapaliysa acar.
 */
void fan_toggle(void);

/**
 * @brief Fanin su anki durumunu dondurur.
 * @return true: acik, false: kapali
 */
bool fan_is_on(void);
