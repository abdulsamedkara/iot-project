#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_heap_caps.h"

#include "config.h"
#include "i2s_mic.h"
#include "i2s_player.h"
#include "voice_client.h"
#include "pomodoro.h"
#include "ui/display.h"
#include "ui/ui_idle.h"
#include "ui/ui_pomodoro.h"
#include "ui/ui_sensor.h"
#include "ui/ui_voice.h"
#include "ui/ui_ai.h"
#include <math.h>
#include <time.h>
#include "esp_sntp.h"

// Sensör kütüphaneleri
#include "esp_adc/adc_oneshot.h"
#include "onewire_bus.h"
#include "ds18b20.h"

static const char *TAG = "main";

// Sensör handle'ları
static adc_oneshot_unit_handle_t adc1_handle;
static ds18b20_device_handle_t ds18b20_handle = NULL;

// Ekran / UI Durum
static screen_id_t current_screen = SCREEN_IDLE;
static float last_temp = 0.0f;
static float last_ldr = 0.0f;

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
    
    // Uzun süren LLM/STT isteklerinde bağlantının kopmaması için güç tasarrufunu devre dışı bırakıyoruz.
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    EventBits_t bits = xEventGroupWaitBits(s_wifi_eg,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi bağlandı: %s", WIFI_SSID);
        return ESP_OK;
    }
    ESP_LOGE(TAG, "WiFi bağlanamadı!");
    return ESP_FAIL;
}

static void sync_time_init(void) {
    ESP_LOGI(TAG, "SNTP başlatılıyor...");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
    setenv("TZ", "TRT-3", 1); // Türkiye saati (UTC+3 sabit)
    tzset();
}

// ─── Buton Yardımcısı ─────────────────────────────────────────────────────────
static void button_init(void)
{
    gpio_config_t gc = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO) | (1ULL << LEFT_BUTTON_GPIO) | (1ULL << RIGHT_BUTTON_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&gc);
}

static inline bool button_pressed(void) { return gpio_get_level(BUTTON_GPIO) == 0; }
static inline bool left_button_pressed(void) { return gpio_get_level(LEFT_BUTTON_GPIO) == 0; }
static inline bool right_button_pressed(void) { return gpio_get_level(RIGHT_BUTTON_GPIO) == 0; }

// ─── Sensör Yardımcıları ──────────────────────────────────────────────────────
static void sensor_init(void)
{
    /* LDR (ADC1) İlklendirme */
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    if (adc_oneshot_new_unit(&init_config1, &adc1_handle) == ESP_OK) {
        adc_oneshot_chan_cfg_t config = {
            .bitwidth = ADC_BITWIDTH_DEFAULT,
            .atten = ADC_ATTEN_DB_12,
        };
        adc_oneshot_config_channel(adc1_handle, LDR_ADC_CHANNEL, &config);
        ESP_LOGI(TAG, "LDR (ADC) hazır.");
    }

    /* DS18B20 İlklendirme */
    onewire_bus_handle_t bus = NULL;
    onewire_bus_config_t bus_config = {
        .bus_gpio_num = DS18B20_GPIO,
    };
    onewire_bus_rmt_config_t rmt_config = {
        .max_rx_bytes = 10,
    };
    
    if (onewire_new_bus_rmt(&bus_config, &rmt_config, &bus) == ESP_OK) {
        onewire_device_iter_handle_t iter = NULL;
        onewire_device_t next_onewire_device;

        onewire_new_device_iter(bus, &iter);
        ESP_LOGI(TAG, "1-Wire cihazları aranıyor...");
        if (onewire_device_iter_get_next(iter, &next_onewire_device) == ESP_OK) {
            ds18b20_config_t ds_cfg = {};
            ds18b20_new_device_from_enumeration(&next_onewire_device, &ds_cfg, &ds18b20_handle);
            ESP_LOGI(TAG, "DS18B20 bulundu");
        } else {
            ESP_LOGW(TAG, "DS18B20 bulunamadı!");
        }
        onewire_del_device_iter(iter);
    }
}

static float read_temperature(void) {
    if (!ds18b20_handle) return 0.0f;
    float temp = 0.0f;
    ds18b20_trigger_temperature_conversion(ds18b20_handle);
    vTaskDelay(pdMS_TO_TICKS(800)); // 12-bit dönüşüm için ~750ms
    ds18b20_get_temperature(ds18b20_handle, &temp);
    return temp;
}

static float read_ldr_percent(void) {
    if (!adc1_handle) return 0.0f;
    
    // ESP32 ADC'si oldukça elektriksel gürültülüdür (Noisy). 
    // Daha stabil değer almak için 64 örneklem alıp ortalamasını alıyoruz.
    long adc_sum = 0;
    int adc_raw = 0;
    for (int i = 0; i < 64; i++) {
        adc_oneshot_read(adc1_handle, LDR_ADC_CHANNEL, &adc_raw);
        adc_sum += adc_raw;
        // Çok kısa bekleme (örn: delayVTaskDelay) yapmıyoruz, arka arkaya okuyoruz
    }
    float raw_avg = (float)adc_sum / 64.0f;
    
    float percent = (raw_avg / 4095.0f) * 100.0f;
    return percent;
}

static float read_noise_level(void) {
    int16_t noise_buf[2048]; // ~128 ms at 16000Hz
    size_t got = 0;
    
    // Kısa bir süre ambient mikrofon verisi oku
    esp_err_t err = i2s_mic_read(noise_buf, 2048, &got, 150);
    if (err != ESP_OK || got == 0) return 0.0f;
    
    // RMS (Ortalama Karekök)
    double sum = 0;
    for (size_t i = 0; i < got; i++) {
        double val = (double)noise_buf[i];
        sum += (val * val);
    }
    double rms = sqrt(sum / got);
    
    // Basit DbFS benzeri logaritmik çevirim. (max ~32768)
    if (rms < 1.0) return 0.0f;
    float noise_db = 20.0 * log10(rms);
    return noise_db;
}

// ─── Akıllı LED (LEDC PWM) ──────────────────────────────────────────────────
static void smart_led_init(void) {
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_13_BIT, // 0-8191
        .freq_hz          = 1000,              // 1 kHz PWM
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = SMART_LED_GPIO,
        .duty           = 0, // Başlangıçta kapalı
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
    ESP_LOGI(TAG, "Akıllı LED GPIO %d PWM yapılandırıldı.", SMART_LED_GPIO);
}

static void smart_led_update(float light_percent) {
    int duty = 0;
    
    // Katmanlı (Stepped) parlaklık mantığı:
    // Çok aydınlıksa tamamen kapalı
    if (light_percent > 60.0f) {
        duty = 0;
    }
    // Hafif loş ortam
    else if (light_percent > 40.0f) {
        duty = 2048; // %25 Parlaklık
    }
    // Karanlık ortam
    else if (light_percent > 20.0f) {
        duty = 4096; // %50 Parlaklık
    }
    // Çok karanlık ortam
    else if (light_percent > 5.0f) {
        duty = 6144; // %75 Parlaklık
    }
    // Zifiri karanlık
    else {
        duty = 8191; // %100 Parlaklık
    }

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

// ─── L298N Fan Kontrolü (PWM Hız Ayarlı) ────────────────────────────────────
static void fan_init(void)
{
    // IN4 yön pini
    gpio_config_t gc = {
        .pin_bit_mask = (1ULL << FAN_IN4_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&gc);
    gpio_set_level(FAN_IN4_GPIO, 0);

    // ENA pini - LEDC PWM ile hız kontrolü
    ledc_timer_config_t fan_timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .timer_num       = FAN_LEDC_TIMER,
        .duty_resolution = FAN_DUTY_RES,
        .freq_hz         = FAN_PWM_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&fan_timer);

    ledc_channel_config_t fan_ch = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = FAN_LEDC_CHANNEL,
        .timer_sel  = FAN_LEDC_TIMER,
        .intr_type  = LEDC_INTR_DISABLE,
        .gpio_num   = FAN_ENA_GPIO,
        .duty       = 0,
        .hpoint     = 0,
    };
    ledc_channel_config(&fan_ch);
    ESP_LOGI(TAG, "Fan: IN4=GPIO%d ENA=GPIO%d (PWM)",
             FAN_IN4_GPIO, FAN_ENA_GPIO);
}

/**
 * @brief Sıcaklığa göre fan hızını ayarlar.
 *        temp < FAN_TEMP_ON   → fan kapalı
 *        FAN_TEMP_ON..MAX     → doğrusal PWM (%0 → %100)
 *        temp >= FAN_TEMP_MAX → fan tam hızda (%100)
 */
static void fan_update(float temp_c)
{
    uint32_t duty = 0;

    if (temp_c >= FAN_TEMP_ON) {
        // Doğrusal interpolasyon: FAN_TEMP_ON → 0%, FAN_TEMP_MAX → 255
        float ratio = (temp_c - FAN_TEMP_ON) / (FAN_TEMP_MAX - FAN_TEMP_ON);
        if (ratio > 1.0f) ratio = 1.0f;
        // Minimum %40 duty ile başla (motorun dönebilmesi için)
        duty = (uint32_t)(51.0f + ratio * (127.0f - 51.0f));

        // Yön: IN4=HIGH (tek yön)
        gpio_set_level(FAN_IN4_GPIO, 1);
        ESP_LOGI(TAG, "[Fan] %.1f°C → duty=%lu (%d%%)",
                 temp_c, duty, (int)(duty * 100 / 255));
    } else {
        // Sıcaklık düşük, fanı durdur
        gpio_set_level(FAN_IN4_GPIO, 0);
        duty = 0;
        ESP_LOGI(TAG, "[Fan] %.1f°C → kapalı", temp_c);
    }

    ledc_set_duty(LEDC_LOW_SPEED_MODE, FAN_LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, FAN_LEDC_CHANNEL);
}

// ─── UI Navigasyon Görevi (Task) ──────────────────────────────────────────────
static uint32_t s_last_interaction_ticks = 0;

static void button_task(void *arg) {
    static bool last_left = false;
    static bool last_right = false;

    while (1) {
        bool cur_left = left_button_pressed();
        bool cur_right = right_button_pressed();
        bool cur_main = button_pressed();
        
        if ((cur_left && !last_left) || (cur_right && !last_right) || cur_main) {
            s_last_interaction_ticks = xTaskGetTickCount();
        }
        
        if (cur_left && !last_left) {
            current_screen = (current_screen == 0) ? SCREEN_AI : (current_screen - 1);
            if (current_screen == SCREEN_VOICE) current_screen = SCREEN_SENSOR; // Skip Voice
            display_switch_screen(current_screen);
        }
        if (cur_right && !last_right) {
            current_screen = (current_screen == SCREEN_AI) ? 0 : (current_screen + 1);
            if (current_screen == SCREEN_VOICE) current_screen = SCREEN_AI; // Skip Voice
            display_switch_screen(current_screen);
        }
        last_left = cur_left;
        last_right = cur_right;

        // Inactivity timeout: auto return to Pomodoro
        if (xTaskGetTickCount() - s_last_interaction_ticks > pdMS_TO_TICKS(15000)) {
            if (current_screen == SCREEN_IDLE || current_screen == SCREEN_SENSOR || current_screen == SCREEN_AI) {
                current_screen = SCREEN_POMODORO;
                display_switch_screen(current_screen);
                s_last_interaction_ticks = xTaskGetTickCount(); // Sürekli tekrar geçiş olmasın
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
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
    sync_time_init();

    /* I2S */
    ESP_ERROR_CHECK(i2s_mic_init());
    ESP_ERROR_CHECK(i2s_player_init(SPK_SAMPLE_RATE, 16, 1));

    /* Buton ve Pomodoro */
    button_init();
    pomodoro_init();
    xTaskCreate(button_task, "btn_task", 2048, NULL, 5, NULL);

    /* Ekran */
    display_init();

    /* Sensörler */
    sensor_init();
    smart_led_init();
    fan_init();

    /* PSRAM tamponları */
    int16_t *rec_buf  = heap_caps_malloc(RECORD_BUF_SIZE, MALLOC_CAP_SPIRAM);
    uint8_t *resp_buf = heap_caps_malloc(RESP_BUF_SIZE,   MALLOC_CAP_SPIRAM);
    if (!rec_buf || !resp_buf) {
        ESP_LOGE(TAG, "PSRAM tahsis hatası!");
        return;
    }

    ESP_LOGI(TAG, "Hazır — butona basılı tutarak konuşun, bırakınca gönderilir.");

    TickType_t last_sensor_update = xTaskGetTickCount();

    while (1) {
        /* Timer: Sensör Okuma ve Proaktif Uyarı Kontrolü */
        if ((xTaskGetTickCount() - last_sensor_update) * portTICK_PERIOD_MS >= SENSOR_UPDATE_MS) {
            float temp = read_temperature();
            float light = read_ldr_percent();
            float noise = read_noise_level();
            ESP_LOGI(TAG, "[Timer] Sensör okundu: %.1f C, Işık: %.1f%%, Gürültü: %.1f dB", temp, light, noise);
            last_sensor_update = xTaskGetTickCount();
            
            // LED Parlaklığını ışığa göre ayarla
            smart_led_update(light);
            fan_update(temp);
            
            last_temp = temp;
            last_ldr = light;
            if (display_lock(200)) {
                ui_sensor_update(last_temp, (int)last_ldr, noise);
                display_unlock();
            }
            
            size_t resp_len = 0;
            esp_err_t err = voice_client_sensor_update(temp, light, noise, resp_buf, RESP_BUF_SIZE, &resp_len);
            if (err == ESP_OK && resp_len > 10) {
                ESP_LOGI(TAG, "▶ Proaktif Uyarı Çalınıyor (Size: %zu)", resp_len);
                i2s_player_play(resp_buf, resp_len);
            }
        }

        /* 1. Buton bekleme ve Pomodoro Kontrolü */
        bool event_triggered = false;
        char pomo_evt[32] = {0};
        static uint32_t last_log_ticks = 0;

        bool pressed = false;
        for (int i=0; i<50; i++) { // ~1 sn lik blokta bekle, timer'a fırsat ver
            if (button_pressed()) {
                pressed = true; break;
            }

            if (pomodoro_check_event(pomo_evt)) {
                event_triggered = true;
                break;
            }
            
            uint32_t now_ticks = xTaskGetTickCount();
            static uint32_t last_disp_ticks = 0;
            
            if (now_ticks - last_disp_ticks > pdMS_TO_TICKS(1000)) {
                uint32_t rem = pomodoro_get_remaining_sec();
                if (display_lock(200)) {
                    if (current_screen == SCREEN_IDLE) {
                        ui_idle_tick();
                    } else if (current_screen == SCREEN_POMODORO) {
                        ui_pomodoro_update(pomodoro_get_state(), rem, pomodoro_get_total_sec(), pomodoro_get_completed_count());
                    }
                    display_unlock();
                }
                last_disp_ticks = now_ticks;
            }

            if (now_ticks - last_log_ticks > pdMS_TO_TICKS(10000)) {
                uint32_t rem = pomodoro_get_remaining_sec();
                if (pomodoro_get_state() != POMO_IDLE) {
                    ESP_LOGI(TAG, "[TIMER] %s Modu (%d. Pomodoro). Kalan Süre: %02lu:%02lu", 
                             pomodoro_get_state_str(), pomodoro_get_completed_count() + 1, rem / 60, rem % 60);
                }
                last_log_ticks = now_ticks;
            }

            vTaskDelay(pdMS_TO_TICKS(20));
        }

        if (event_triggered) {
            ESP_LOGW(TAG, ">> Pomodoro Olayı Tetiklendi: %s", pomo_evt);
            
            // Ekranı Pomodoro'ya çek (Otomatik geçiş görünsün)
            current_screen = SCREEN_POMODORO;
            display_switch_screen(current_screen);
            s_last_interaction_ticks = xTaskGetTickCount();

            // Durum değişti, UI'ı hemen güncelle (animasyon + zaman anında yansısın)
            if (display_lock(200)) {
                ui_pomodoro_update(pomodoro_get_state(), pomodoro_get_remaining_sec(),
                                   pomodoro_get_total_sec(), pomodoro_get_completed_count());
                display_unlock();
            }

            size_t resp_len = 0;
            char action_out[32] = {0};
            static char transcript_out[1024];
            static char answer_out[2048];
            transcript_out[0] = '\0';
            answer_out[0] = '\0';
            
            // Ses verisi yok, sadece header ile STT atlayıp LLM cevabı alıyoruz.
            esp_err_t ret = voice_client_transcribe(
                NULL, 0,
                pomo_evt, pomodoro_get_state_str(),
                resp_buf, RESP_BUF_SIZE, &resp_len, action_out,
                transcript_out, sizeof(transcript_out),
                answer_out, sizeof(answer_out)
            );
            
            if (ret == ESP_OK && resp_len > 10) {
                ESP_LOGI(TAG, "▶ Olay yanıtı çalınıyor (Size: %zu)", resp_len);
                i2s_player_play(resp_buf, resp_len);
            } else {
                ESP_LOGE(TAG, "Olay için sunucudan yanıt alınamadı.");
            }
            continue;
        }

        if (!pressed) continue;

        vTaskDelay(pdMS_TO_TICKS(50)); /* debounce */
        if (!button_pressed()) continue;

        ESP_LOGI(TAG, "◉ Kayıt başladı...");
        current_screen = SCREEN_VOICE;
        display_switch_screen(current_screen);
        if (display_lock(200)) {
            ui_voice_set_state(VOICE_RECORDING);
            ui_voice_set_transcript("...");
            display_unlock();
        }

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
        current_screen = SCREEN_AI;
        display_switch_screen(current_screen);
        if (display_lock(200)) {
            ui_ai_set_thinking(true);
            ui_ai_set_response("...", "Pomodoro Chatbot");
            display_unlock();
        }
        size_t resp_len = 0;
        char action_out[32] = {0};
        static char transcript_out[1024];
        static char answer_out[2048];
        transcript_out[0] = '\0';
        answer_out[0] = '\0';
        
        ret = voice_client_transcribe(
            (uint8_t *)rec_buf, pcm_bytes,
            NULL, pomodoro_get_state_str(),
            resp_buf, RESP_BUF_SIZE, &resp_len, action_out,
            transcript_out, sizeof(transcript_out),
            answer_out, sizeof(answer_out)
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
        // Güvenlik: Transkript içinde pomodoro kelimesi geçmiyorsa action'ı yoksay
        bool pomo_keyword_found = (strcasestr(transcript_out, "pomodoro") != NULL ||
                                   strcasestr(transcript_out, "sayac") != NULL ||
                                   strcasestr(transcript_out, "calis") != NULL ||
                                   strcasestr(transcript_out, "durdur") != NULL ||
                                   strcasestr(transcript_out, "basla") != NULL ||
                                   strcasestr(transcript_out, "baslat") != NULL ||
                                   strcasestr(transcript_out, "devam") != NULL ||
                                   strcasestr(transcript_out, "yeniden") != NULL ||
                                   strcasestr(transcript_out, "mola") != NULL ||
                                   strcasestr(transcript_out, "sure") != NULL ||
                                   strcasestr(transcript_out, "süre") != NULL);
        if (strlen(action_out) > 0 && !pomo_keyword_found) {
            ESP_LOGW(TAG, "Action '%s' alindi ama transkriptte pomodoro kelimesi yok, yoksayildi.", action_out);
            action_out[0] = '\0'; // Temizle
        }
        if (strcmp(action_out, "POMO_START") == 0) {
            if (pomodoro_start()) {
                ESP_LOGW(TAG, "========= POMODORO ÇALIŞMASI BAŞLADI =========");
                current_screen = SCREEN_POMODORO;
                display_switch_screen(current_screen);
            } else {
                ESP_LOGI(TAG, "Pomodoro zaten aktif, yeniden başlatılmadı.");
            }
        } else if (strcmp(action_out, "POMO_STOP") == 0) {
            pomodoro_stop();
            ESP_LOGW(TAG, "========= POMODORO SAYACI DURDURULDU =========");
            current_screen = SCREEN_POMODORO;
            display_switch_screen(current_screen);
        }

        if (display_lock(200)) {
            ui_ai_set_thinking(false);
            if (strlen(answer_out) > 0) {
                ui_ai_set_response(answer_out, strlen(transcript_out) > 0 ? transcript_out : "Sistem");
            } else {
                ui_ai_set_response(action_out, "Tamamlandı");
            }
            display_unlock();
        }
        // Cevap geldi, inactivity sayacını sıfırla (15sn'den önce Pomodoro'ya geçmesin)
        s_last_interaction_ticks = xTaskGetTickCount();
        ESP_LOGI(TAG, "✓ Tamamlandı.\n");
    }
}