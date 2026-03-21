#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_heap_caps.h"

#include "config.h"
#include "i2s_mic.h"
#include "i2s_player.h"
#include "voice_client.h"
#include "pomodoro.h"
#include "display.h"

static const char *TAG = "main";

// ─── WiFi ─────────────────────────────────────────────────────────────────────
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static EventGroupHandle_t s_wifi_eg;
static int s_retry = 0;

static void wifi_event_handler(void *arg, esp_event_base_t base,
                                int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            s_retry++;
            ESP_LOGW(TAG, "WiFi yeniden bağlanıyor (%d/%d)...", s_retry, WIFI_MAX_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_eg, WIFI_FAIL_BIT);
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "IP: " IPSTR, IP2STR(&ev->ip_info.ip));
        s_retry = 0;
        xEventGroupSetBits(s_wifi_eg, WIFI_CONNECTED_BIT);
    }
}

static esp_err_t wifi_init_sta(void)
{
    s_wifi_eg = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t eh_wifi, eh_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &eh_wifi));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &eh_ip));

    wifi_config_t wc = {
        .sta = {
            .ssid     = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wc));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(s_wifi_eg,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi bağlandı: %s", WIFI_SSID);
        return ESP_OK;
    }
    ESP_LOGE(TAG, "WiFi bağlanamadı!");
    return ESP_FAIL;
}

// ─── Buton Yardımcısı ─────────────────────────────────────────────────────────
static void button_init(void)
{
    gpio_config_t gc = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&gc);
}

static inline bool button_pressed(void)
{
    return gpio_get_level(BUTTON_GPIO) == 0; /* pull-up → basılı = LOW */
}

// ─── Ana Uygulama ─────────────────────────────────────────────────────────────
void app_main(void)
{
    /* NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* WiFi */
    ESP_ERROR_CHECK(wifi_init_sta());

    /* I2S */
    ESP_ERROR_CHECK(i2s_mic_init());
    ESP_ERROR_CHECK(i2s_player_init(SPK_SAMPLE_RATE, 16, 1));

    /* Buton */
    button_init();
    pomodoro_init();
    
    /* Ekran */
    display_init();
    display_update_status("Hazır");

    /* PSRAM tamponları */
    int16_t *rec_buf  = heap_caps_malloc(RECORD_BUF_SIZE, MALLOC_CAP_SPIRAM);
    uint8_t *resp_buf = heap_caps_malloc(RESP_BUF_SIZE,   MALLOC_CAP_SPIRAM);
    if (!rec_buf || !resp_buf) {
        ESP_LOGE(TAG, "PSRAM tahsis hatası!");
        return;
    }

    ESP_LOGI(TAG, "Hazır — butona basılı tutarak konuşun, bırakınca gönderilir.");

    while (1) {
        /* 1. Buton bekleme ve Pomodoro Kontrolü */
        bool event_triggered = false;
        char pomo_evt[32] = {0};
        static uint32_t last_log_ticks = 0;

        while (!button_pressed()) {
            if (pomodoro_check_event(pomo_evt)) {
                event_triggered = true;
                break;
            }
            
            uint32_t now_ticks = xTaskGetTickCount();
            static uint32_t last_disp_ticks = 0;
            
            if (now_ticks - last_disp_ticks > pdMS_TO_TICKS(1000)) {
                uint32_t rem = pomodoro_get_remaining_sec();
                // 1 saniyede bir ekrani guncelle
                display_update_timer(rem / 60, rem % 60);
                display_update_status(pomodoro_get_state_str());
                last_disp_ticks = now_ticks;
            }

            if (now_ticks - last_log_ticks > pdMS_TO_TICKS(10000)) {
                uint32_t rem = pomodoro_get_remaining_sec();
                if (pomodoro_get_state() != POMO_IDLE) {
                    ESP_LOGI(TAG, "[TIMER] %s Modu. Kalan Süre: %02lu:%02lu", 
                             pomodoro_get_state_str(), rem / 60, rem % 60);
                }
                last_log_ticks = now_ticks;
            }
            

            vTaskDelay(pdMS_TO_TICKS(50));
        }

        if (event_triggered) {
            ESP_LOGW(TAG, ">> Pomodoro Olayı Tetiklendi: %s", pomo_evt);
            size_t resp_len = 0;
            char action_out[32] = {0};
            
            // Ses verisi yok, sadece header ile STT atlayıp LLM cevabı alıyoruz.
            esp_err_t ret = voice_client_transcribe(
                NULL, 0,
                pomo_evt, pomodoro_get_state_str(),
                resp_buf, RESP_BUF_SIZE, &resp_len, action_out
            );
            
            if (ret == ESP_OK && resp_len > 10) {
                ESP_LOGI(TAG, "▶ Olay yanıtı çalınıyor (Size: %zu)", resp_len);
                i2s_player_play(resp_buf, resp_len);
            } else {
                ESP_LOGE(TAG, "Olay için sunucudan yanıt alınamadı.");
            }
            continue;
        }

        vTaskDelay(pdMS_TO_TICKS(50)); /* debounce */
        if (!button_pressed()) continue;

        ESP_LOGI(TAG, "◉ Kayıt başladı...");
        display_update_status("Dinliyor...");

        /* 2. Kayıt */
        size_t total_samples = 0;
        const size_t CHUNK   = 512;
        const size_t MAX_SAMP = RECORD_BUF_SIZE / sizeof(int16_t);

        while (button_pressed() && total_samples + CHUNK <= MAX_SAMP) {
            size_t got = 0;
            i2s_mic_read(rec_buf + total_samples, CHUNK, &got, 200);
            total_samples += got;
        }

        size_t pcm_bytes = total_samples * sizeof(int16_t);
        ESP_LOGI(TAG, "■ Kayıt bitti: %.2f sn (%zu bayt)",
                 (float)total_samples / MIC_SAMPLE_RATE, pcm_bytes);

        if (pcm_bytes < MIC_SAMPLE_RATE / 2) { /* < 0.5 sn */
            ESP_LOGW(TAG, "Çok kısa, atlanıyor.");
            continue;
        }

        /* 3. Sunucuya gönder */
        ESP_LOGI(TAG, "⬆ Sunucuya gönderiliyor...");
        display_update_status("Düşünüyor...");
        size_t resp_len = 0;
        char action_out[32] = {0};
        
        ret = voice_client_transcribe(
            (uint8_t *)rec_buf, pcm_bytes,
            NULL, pomodoro_get_state_str(),
            resp_buf, RESP_BUF_SIZE, &resp_len, action_out
        );

        if (ret != ESP_OK || resp_len < 10) {
            ESP_LOGE(TAG, "Sunucu yanıtı alınamadı veya çok kısa: %zu", resp_len);
            continue;
        }

        /* 4. Doğrudan PCM Olarak Çal */
        ESP_LOGI(TAG, "▶ Çalınıyor: %luHz 16bit 1ch (Size: %zu)",
                 (unsigned long)SPK_SAMPLE_RATE, resp_len);

        i2s_player_play(resp_buf, resp_len);
        
        /* 5. Eylem kontrolü (Pomodoro) */
        if (strcmp(action_out, "POMO_START") == 0) {
            if (pomodoro_start()) {
                ESP_LOGW(TAG, "========= POMODORO ÇALIŞMASI BAŞLADI =========");
            } else {
                ESP_LOGI(TAG, "Pomodoro zaten aktif, yeniden başlatılmadı.");
            }
        } else if (strcmp(action_out, "POMO_STOP") == 0) {
            pomodoro_stop();
            ESP_LOGW(TAG, "========= POMODORO SAYACI DURDURULDU =========");
        }

        display_update_status(pomodoro_get_state() == POMO_IDLE ? "Hazır" : pomodoro_get_state_str());
        ESP_LOGI(TAG, "✓ Tamamlandı.\n");
    }
}