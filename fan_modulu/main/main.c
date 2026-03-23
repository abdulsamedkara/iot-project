/**
 * fan_modulu - Bagimsiz Fan Motor Test Projesi
 *
 * Bu proje, ana projeye dokunmadan L298N motor surucu
 * ile fan kontrolunu test etmek icin olusturulmustur.
 *
 * Test Senaryolari:
 *   1. Otomatik mod: Simule edilmis sicaklik degerine gore
 *      28C esigini asan sicaklikta fan otomatik acar.
 *   2. Sesli komut simulasyonu: Her 10 saniyede FAN_ON / FAN_OFF
 *      action'larini simule ederek toggle test yapar.
 *   3. Manuel override: Sesli komut geldiginde,
 *      sicaklik etkisi devre disi kalir.
 *
 * Baglanti:
 *   L298N IN4 --> GPIO 18
 *   L298N IN3 --> GND
 *   L298N GND --> ESP32 GND (ZORUNLU!)
 *   L298N VCC --> Pil (+)
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "fan.h"
#include "config.h"

static const char *TAG = "fan_test";

// ─── Durum Makinesi ────────────────────────────────────────────────────────────
// Sesli komut geldiginde override aktif olur.
// Override aktifken sicaklik kontrolu devreye girmez.
// 30 saniye sonra override sona erer, tekrar otomatik moda geri doner.
#define MANUAL_OVERRIDE_TIMEOUT_MS  30000

static bool  s_manual_override     = false;
static int64_t s_override_start_ms = 0;

static int64_t get_ms(void) {
    return (int64_t)xTaskGetTickCount() * portTICK_PERIOD_MS;
}

// ─── Sesli Komut Isleyici ──────────────────────────────────────────────────────
// Ana projede bu fonksiyon action_out string'i sunucudan alir.
// Burada elle cagirarak simule ediyoruz.
static void handle_voice_action(const char *action)
{
    ESP_LOGI(TAG, "[VOICE] Gelen aksiyon: \"%s\"", action);

    if (strcmp(action, "FAN_ON") == 0) {
        fan_on();
        s_manual_override  = true;
        s_override_start_ms = get_ms();
        ESP_LOGW(TAG, "[OVERRIDE] Manuel mod aktif - sicaklik kontrolu %d sn. askiya alindi.",
                 MANUAL_OVERRIDE_TIMEOUT_MS / 1000);
    }
    else if (strcmp(action, "FAN_OFF") == 0) {
        fan_off();
        s_manual_override  = true;
        s_override_start_ms = get_ms();
        ESP_LOGW(TAG, "[OVERRIDE] Manuel mod aktif - sicaklik kontrolu %d sn. askiya alindi.",
                 MANUAL_OVERRIDE_TIMEOUT_MS / 1000);
    }
}

// ─── Sicakliga Gore Otomatik Kontrol ──────────────────────────────────────────
static void handle_auto_temp(float temp)
{
    // Override aktifse, timeout dolana kadar dokunma
    if (s_manual_override) {
        int64_t elapsed = get_ms() - s_override_start_ms;
        if (elapsed < MANUAL_OVERRIDE_TIMEOUT_MS) {
            ESP_LOGI(TAG, "[AUTO] Override aktif (%lld/%d ms), otomatik mod bekleniyor.",
                     elapsed, MANUAL_OVERRIDE_TIMEOUT_MS);
            return;
        }
        // Timeout doldu, tekrar otomatik moda don
        s_manual_override = false;
        ESP_LOGW(TAG, "[OVERRIDE] Sona erdi, otomatik moda donuldu.");
    }

    if (temp >= FAN_TEMP_THRESHOLD) {
        if (!fan_is_on()) {
            ESP_LOGW(TAG, "[AUTO] Sicaklik %.1f C >= %.1f C esigi -> FAN ACILIYOR",
                     temp, FAN_TEMP_THRESHOLD);
            fan_on();
        }
    } else {
        if (fan_is_on()) {
            ESP_LOGW(TAG, "[AUTO] Sicaklik %.1f C < %.1f C esigi -> FAN KAPATILIYOR",
                     temp, FAN_TEMP_THRESHOLD);
            fan_off();
        }
    }
}

// ─── Ana Gorur ────────────────────────────────────────────────────────────────
void app_main(void)
{
    ESP_LOGI(TAG, "=== fan_modulu Test Basladi ===");
    ESP_LOGI(TAG, "FAN_IN4_GPIO     : %d", FAN_IN4_GPIO);
    ESP_LOGI(TAG, "FAN_TEMP_ESIGI   : %.1f C", FAN_TEMP_THRESHOLD);

    fan_init();

    // ──────────────────────────────────────────────────────────────────────────
    // TEST SENARYOSU:
    //
    //  0-30 sn  : Simule sicaklik = 25 C  -> Fan kapali olmali (esik alti)
    // 30-60 sn  : Simule sicaklik = 32 C  -> Fan otomatik acilmali
    // 60. sn    : Sesli komut: FAN_OFF    -> Fan kapanmali (override basladi)
    // 61-90 sn  : Sicaklik hala 32 C ama override aktif -> Fan kapali kalmali
    // 90. sn    : Override sona erer, sicaklik 32C -> Otomatik tekrar acar
    //120-150 sn : Sesli komut: FAN_ON     -> Override ile manuel acilir
    //150-180 sn : Simule sicaklik = 20 C  -> Override bitti sonra otomatik kapanir
    // ──────────────────────────────────────────────────────────────────────────

    TickType_t start_ticks = xTaskGetTickCount();
    int  loop_count  = 0;
    bool fired_off   = false;
    bool fired_on2   = false;

    while (1) {
        int64_t elapsed_s = (int64_t)(xTaskGetTickCount() - start_ticks)
                            * portTICK_PERIOD_MS / 1000;

        // Simule sicaklik
        float sim_temp;
        if (elapsed_s < 30) {
            sim_temp = 25.0f;
        } else if (elapsed_s < 150) {
            sim_temp = 32.0f;
        } else {
            sim_temp = 20.0f;
        }

        // Simule sesli komut olaylari (tek seferlik)
        if (elapsed_s >= 60 && !fired_off) {
            fired_off = true;
            handle_voice_action("FAN_OFF"); // Kullanici "Fani kapat" dedi
        }
        if (elapsed_s >= 120 && !fired_on2) {
            fired_on2 = true;
            handle_voice_action("FAN_ON"); // Kullanici "Fani ac" dedi
        }

        // Otomatik sicaklik kontrolu
        handle_auto_temp(sim_temp);

        // 5 saniyede bir ozet log
        if (loop_count % 10 == 0) {
            ESP_LOGI(TAG, "─────────────────────────────────────────────────");
            ESP_LOGI(TAG, "Gecen sure  : %lld sn", elapsed_s);
            ESP_LOGI(TAG, "Sim. sicaklik: %.1f C  (Esik: %.1f C)", sim_temp, FAN_TEMP_THRESHOLD);
            ESP_LOGI(TAG, "Fan durumu  : %s", fan_is_on() ? "ACIK ✓" : "KAPALI ✗");
            ESP_LOGI(TAG, "Override    : %s", s_manual_override ? "AKTIF" : "PASIF");
            ESP_LOGI(TAG, "─────────────────────────────────────────────────");
        }

        loop_count++;
        vTaskDelay(pdMS_TO_TICKS(500)); // Her 500ms kontrol
    }
}
