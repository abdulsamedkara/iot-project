#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "esp_timer.h"

#include "driver/i2s_std.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#include "onewire_bus.h"
#include "ds18b20.h"

#define WIFI_SSID       "FiberHGW_Kara_2.4GHz"
#define WIFI_PASS       "12312010kara"
#define SERVER_URL      "http://192.168.1.117:5000/chat"

#define SAMPLE_RATE         16000
#define RECORD_TIME_SEC     3
#define TOTAL_SAMPLES       (SAMPLE_RATE * RECORD_TIME_SEC)
#define AUDIO_BUFFER_SIZE   (TOTAL_SAMPLES * sizeof(int16_t))

#define I2S_MIC_SD      40
#define I2S_MIC_SCK     41
#define I2S_MIC_WS      42

// Sensör Pinleri
#define ONEWIRE_GPIO    4
#define LDR_ADC_CHAN    ADC_CHANNEL_6 // GPIO 7 is ADC1 Channel 6 on ESP32-S3

// Sunucu cevabı için buffer
#define RESPONSE_BUFFER_SIZE 2048

static const char *TAG = "JARVIS";

static volatile bool wifi_connected = false;

static i2s_chan_handle_t rx_handle = NULL;
static i2s_chan_handle_t tx_handle = NULL;

static char response_buffer[RESPONSE_BUFFER_SIZE];
static int response_len = 0;

// Sensör Değişkenleri
static adc_oneshot_unit_handle_t adc1_handle;
static onewire_bus_handle_t onewire_bus;
static ds18b20_device_handle_t ds18b20_device;

static float current_temp_c = 0.0f;
static int current_ldr_val = 0;
static int current_noise_db = 0;

// ---------------------------------------------------------------------------
// Pomodoro State Machine
// ---------------------------------------------------------------------------
typedef enum {
    POMODORO_IDLE,
    POMODORO_RUNNING,
    POMODORO_PAUSED
} pomodoro_state_t;

static volatile pomodoro_state_t pomodoro_state = POMODORO_IDLE;
static volatile int pomodoro_remaining_sec = 0;
static TaskHandle_t pomodoro_task_handle = NULL;


static void pomodoro_countdown_task(void *pvParameters)
{
    int duration_sec = (int)(intptr_t)pvParameters;
    pomodoro_remaining_sec = duration_sec;
    pomodoro_state = POMODORO_RUNNING;

    ESP_LOGI(TAG, "=== POMODORO BASLADI: %d dakika ===", duration_sec / 60);

    while (pomodoro_remaining_sec > 0 && pomodoro_state == POMODORO_RUNNING) {
        int mins = pomodoro_remaining_sec / 60;
        int secs = pomodoro_remaining_sec % 60;

        ESP_LOGI(TAG, "POMODORO: %02d:%02d", mins, secs);

        // TODO: ILI9341 TFT uyarlandığında güncellenecek
        vTaskDelay(pdMS_TO_TICKS(1000));
        pomodoro_remaining_sec--;
    }

    if (pomodoro_remaining_sec <= 0) {
        ESP_LOGI(TAG, "=== POMODORO TAMAMLANDI! ===");
    } else {
        ESP_LOGI(TAG, "=== POMODORO DURDURULDU ===");
    }

    pomodoro_state = POMODORO_IDLE;
    pomodoro_remaining_sec = 0;
    pomodoro_task_handle = NULL;
    vTaskDelete(NULL);
}


static void pomodoro_start(int duration_min)
{
    if (pomodoro_task_handle != NULL) {
        pomodoro_state = POMODORO_IDLE;
        vTaskDelay(pdMS_TO_TICKS(100));
        if (pomodoro_task_handle != NULL) {
            vTaskDelete(pomodoro_task_handle);
            pomodoro_task_handle = NULL;
        }
    }

    int duration_sec = duration_min * 60;
    xTaskCreate(
        pomodoro_countdown_task,
        "pomodoro_task",
        4096,
        (void *)(intptr_t)duration_sec,
        3,
        &pomodoro_task_handle
    );
}


static void pomodoro_stop(void)
{
    if (pomodoro_state == POMODORO_RUNNING && pomodoro_task_handle != NULL) {
        pomodoro_state = POMODORO_IDLE;
        ESP_LOGI(TAG, "Pomodoro durdurma komutu alindi");
    } else {
        ESP_LOGI(TAG, "Pomodoro zaten calismiyordu");
    }
}


// ---------------------------------------------------------------------------
// Sensör Başlatma ve Okuma
// ---------------------------------------------------------------------------
static void sensors_init(void)
{
    // ADC1 (LDR) Init
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, LDR_ADC_CHAN, &config));
    ESP_LOGI(TAG, "LDR ADC1 baslatildi (Kanal %d, GPIO 7)", LDR_ADC_CHAN);

    // OneWire DS18B20 Init
    onewire_bus_config_t bus_config = {
        .bus_gpio_num = ONEWIRE_GPIO,
    };
    onewire_bus_rmt_config_t rmt_config = {
        .max_rx_bytes = 10,
    };
    if (onewire_new_bus_rmt(&bus_config, &rmt_config, &onewire_bus) == ESP_OK) {
        ESP_LOGI(TAG, "OneWire RMT bus olusturuldu (GPIO %d)", ONEWIRE_GPIO);
        
        onewire_device_iter_handle_t iter = NULL;
        onewire_device_t next_onewire_device;
        bool ds18b20_found = false;

        ESP_ERROR_CHECK(onewire_new_device_iter(onewire_bus, &iter));
        
        while (onewire_device_iter_get_next(iter, &next_onewire_device) == ESP_OK) {
            if (next_onewire_device.address == 0) continue;
            ds18b20_config_t ds_cfg = {};
            if (ds18b20_new_device_from_enumeration(&next_onewire_device, &ds_cfg, &ds18b20_device) == ESP_OK) {
                ds18b20_set_resolution(ds18b20_device, DS18B20_RESOLUTION_12B);
                ESP_LOGI(TAG, "DS18B20 sensoru bulundu!");
                ds18b20_found = true;
                break;
            }
        }
        ESP_ERROR_CHECK(onewire_del_device_iter(iter));

        if (!ds18b20_found) {
            ESP_LOGW(TAG, "DS18B20 sensoru bulunamadi!");
            ds18b20_device = NULL;
        }
    } else {
        ESP_LOGE(TAG, "OneWire bus olusturulamadi");
    }
}

static void read_sensors(void)
{
    // LDR Okuma (ters orantili olarak karanlik 0, aydinlik yüksek olacak sekilde)
    int adc_val = 0;
    if (adc_oneshot_read(adc1_handle, LDR_ADC_CHAN, &adc_val) == ESP_OK) {
        current_ldr_val = adc_val;
        ESP_LOGI(TAG, "LDR degeri: %d", current_ldr_val);
    }

    // DS18B20 Sicaklik Okuma
    if (ds18b20_device != NULL) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(ds18b20_trigger_temperature_conversion(ds18b20_device));
        vTaskDelay(pdMS_TO_TICKS(800)); // DS18B20 cevrim suresi 750ms
        
        float temp = 0.0f;
        if (ds18b20_get_temperature(ds18b20_device, &temp) == ESP_OK) {
            current_temp_c = temp;
            ESP_LOGI(TAG, "DS18B20 Sicaklik: %.2fC", current_temp_c);
        }
    } else {
        current_temp_c = 25.0f; // Sensor yoksa default degere donsun
    }
}

// ---------------------------------------------------------------------------
// Gürültü Seviyesi Hesaplama
// ---------------------------------------------------------------------------
static void calculate_noise(int16_t *audio_buffer, size_t samples)
{
    double sum_sq = 0;
    for (size_t i = 0; i < samples; i++) {
        sum_sq += (double)audio_buffer[i] * audio_buffer[i];
    }
    double rms = sqrt(sum_sq / samples);
    
    // RMS'i basit bir dB seviyesine donustur (0-120 araligi)
    // - referans noktasını mikrofona göre ayarladık
    if (rms > 0) {
        current_noise_db = (int)(20.0 * log10(rms)) - 20; 
        if (current_noise_db < 0) current_noise_db = 0;
    } else {
        current_noise_db = 0;
    }
    ESP_LOGI(TAG, "Hesaplanan Gurultu: %d dB (RMS: %.2f)", current_noise_db, rms);
}


// ---------------------------------------------------------------------------
// Sunucu Cevabını Yorumlama
// ---------------------------------------------------------------------------
static void handle_server_response(const char *response)
{
    if (!response || strlen(response) == 0) {
        ESP_LOGW(TAG, "Bos sunucu cevabi");
        return;
    }

    ESP_LOGI(TAG, "Sunucu cevabi: %s", response);

    if (strstr(response, "POMODORO_START_25") != NULL) {
        ESP_LOGI(TAG, ">> POMODORO_START_25 algilandi!");
        pomodoro_start(25);
        return;
    }

    if (strstr(response, "POMODORO_STOP") != NULL) {
        ESP_LOGI(TAG, ">> POMODORO_STOP algilandi!");
        pomodoro_stop();
        return;
    }

    printf("\n========================================\n");
    printf("ASISTAN: %s\n", response);
    printf("========================================\n");
}


// ---------------------------------------------------------------------------
// HTTP Event Handler
// ---------------------------------------------------------------------------
static bool is_wav_response = false;
static int wav_header_offset = 0;

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP baglandi");
            is_wav_response = false;
            wav_header_offset = 0;
            break;

        case HTTP_EVENT_ON_HEADER:
            if (strcasecmp(evt->header_key, "Content-Type") == 0) {
                if (strstr(evt->header_value, "audio/wav") != NULL) {
                    is_wav_response = true;
                    ESP_LOGI(TAG, "WAV ses yaniti algilandi, stream basliyor...");
                }
            } else if (strcasecmp(evt->header_key, "X-Command") == 0) {
                if (strstr(evt->header_value, "POMODORO_START_25") != NULL) {
                    ESP_LOGI(TAG, ">> POMODORO_START_25 algilandi (Header)!");
                    pomodoro_start(25);
                } else if (strstr(evt->header_value, "POMODORO_STOP") != NULL) {
                    ESP_LOGI(TAG, ">> POMODORO_STOP algilandi (Header)!");
                    pomodoro_stop();
                }
            }
            break;

        case HTTP_EVENT_ON_DATA:
            if (evt->data && evt->data_len > 0) {
                if (is_wav_response) {
                    // WAV header genelde 44 byte'tir. Ilk 44 byte atlanmali
                    int data_len = evt->data_len;
                    uint8_t *data_ptr = (uint8_t *)evt->data;

                    if (wav_header_offset < 44) {
                        int skip_bytes = 44 - wav_header_offset;
                        if (data_len <= skip_bytes) {
                            wav_header_offset += data_len;
                            return ESP_OK; // Tümü header'di, atla
                        } else {
                            wav_header_offset += skip_bytes;
                            data_ptr += skip_bytes;
                            data_len -= skip_bytes;
                        }
                    }

                    // Sesi I2S uzerinden MAX98357A'ya yaz
                    size_t bytes_written = 0;
                    esp_err_t err = i2s_channel_write(tx_handle, data_ptr, data_len, &bytes_written, portMAX_DELAY);
                    if (err != ESP_OK) {
                        ESP_LOGE(TAG, "I2S yazma hatasi: %s", esp_err_to_name(err));
                    }
                } else {
                    // Metin cevabi (Pomodoro vs.)
                    if (response_len + evt->data_len < RESPONSE_BUFFER_SIZE - 1) {
                        memcpy(response_buffer + response_len, evt->data, evt->data_len);
                        response_len += evt->data_len;
                        response_buffer[response_len] = '\0';
                    } else {
                        ESP_LOGW(TAG, "Cevap buffer'i doldu");
                    }
                }
            }
            break;

        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP baglantisi kapandi");
            break;

        default:
            break;
    }
    return ESP_OK;
}


// ---------------------------------------------------------------------------
// Wi-Fi On/Off Control
// ---------------------------------------------------------------------------
static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Wi-Fi basladi, baglaniliyor...");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_connected = false;
        ESP_LOGW(TAG, "Wi-Fi koptu, tekrar baglaniliyor...");
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "IP alindi: " IPSTR, IP2STR(&event->ip_info.ip));
        wifi_connected = true;
    }
}


static void wifi_init_sta(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL));

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));

    strncpy((char *)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password) - 1);

    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi init tamam");
}


// ---------------------------------------------------------------------------
// Hoparlör I2S (MAX98357A) ve Mikrofon I2S (INMP441)
// ---------------------------------------------------------------------------
#define SPK_BCLK    15
#define SPK_LRC     16
#define SPK_DIN     17

static void audio_init(void)
{
    // -----------------------------------------------------------------------
    // RX (Mikrofon) Kanalı -> I2S_NUM_0
    // -----------------------------------------------------------------------
    i2s_chan_config_t rx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&rx_chan_cfg, NULL, &rx_handle));

    i2s_std_slot_config_t rx_slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO);
    rx_slot_cfg.slot_mask = I2S_STD_SLOT_RIGHT;
    i2s_std_config_t rx_std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = rx_slot_cfg,
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_MIC_SCK,
            .ws   = I2S_MIC_WS,
            .dout = I2S_GPIO_UNUSED,
            .din  = I2S_MIC_SD,
            .invert_flags = {
                .mclk_inv = false, .bclk_inv = false, .ws_inv = false,
            },
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle, &rx_std_cfg));

    // -----------------------------------------------------------------------
    // TX (Hoparlör) Kanalı -> I2S_NUM_1
    // Piper 22050Hz dönüyor.
    // -----------------------------------------------------------------------
    i2s_chan_config_t tx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&tx_chan_cfg, &tx_handle, NULL));

    i2s_std_slot_config_t tx_slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO);
    i2s_std_config_t tx_std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(22050), 
        .slot_cfg = tx_slot_cfg,
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = SPK_BCLK,
            .ws   = SPK_LRC,
            .dout = SPK_DIN,
            .din  = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false, .bclk_inv = false, .ws_inv = false,
            },
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle, &tx_std_cfg));
    
    // Hatları başlat
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_handle));

    ESP_LOGI(TAG, "Mikrofon (RX) ve Hoparlor (TX) I2S ayri birimlerde hazir");
}


// ---------------------------------------------------------------------------
// Ses kayıt
// ---------------------------------------------------------------------------
static esp_err_t record_audio_16bit(int16_t *audio_buffer, size_t *out_bytes)
{
    int32_t *raw_chunk = malloc(1024 * sizeof(int32_t));
    if (!raw_chunk) {
        ESP_LOGE(TAG, "raw_chunk ayirilamadi");
        return ESP_ERR_NO_MEM;
    }

    size_t bytes_read = 0;
    size_t samples_written = 0;

    ESP_ERROR_CHECK(i2s_channel_disable(rx_handle));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));

    ESP_LOGI(TAG, "Kayit basliyor: %d saniye", RECORD_TIME_SEC);

    while (samples_written < TOTAL_SAMPLES) {
        esp_err_t ret = i2s_channel_read(
            rx_handle,
            raw_chunk,
            1024 * sizeof(int32_t),
            &bytes_read,
            portMAX_DELAY
        );

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "i2s okuma hatasi: %s", esp_err_to_name(ret));
            free(raw_chunk);
            return ret;
        }

        size_t samples_read = bytes_read / sizeof(int32_t);

        for (size_t i = 0; i < samples_read && samples_written < TOTAL_SAMPLES; i++) {
            int32_t s = raw_chunk[i] >> 8;
            if (s > 32767) s = 32767;
            if (s < -32768) s = -32768;
            audio_buffer[samples_written++] = (int16_t)s;
        }
    }

    *out_bytes = samples_written * sizeof(int16_t);

    free(raw_chunk);

    calculate_noise(audio_buffer, samples_written); // RMS ile gurultu seviyesi

    ESP_LOGI(TAG, "Kayit bitti. Toplam byte: %u", (unsigned)*out_bytes);
    return ESP_OK;
}


// ---------------------------------------------------------------------------
// Ses Gonder & Cevap Al
// ---------------------------------------------------------------------------
static esp_err_t send_audio_to_server(const int16_t *audio_buffer, size_t audio_bytes)
{
    memset(response_buffer, 0, RESPONSE_BUFFER_SIZE);
    response_len = 0;

    esp_http_client_config_t config = {
        .url = SERVER_URL,
        .event_handler = http_event_handler,
        .timeout_ms = 60000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "HTTP client olusturulamadi");
        return ESP_FAIL;
    }

    esp_err_t ret;

    ret = esp_http_client_set_method(client, HTTP_METHOD_POST);
    if (ret != ESP_OK) goto cleanup;

    ret = esp_http_client_set_header(client, "Content-Type", "application/octet-stream");
    if (ret != ESP_OK) goto cleanup;

    // Gerçek sensör degerlerini gonder
    char temp_str[16];
    char ldr_str[16];
    char noise_str[16];
    
    snprintf(temp_str, sizeof(temp_str), "%.1fC", current_temp_c);
    snprintf(ldr_str, sizeof(ldr_str), "%d", current_ldr_val);
    snprintf(noise_str, sizeof(noise_str), "%d", current_noise_db);

    ESP_LOGI(TAG, "Giden Sensor Verileri -> Sicaklik: %s, Isik: %s, Gurultu: %s", temp_str, ldr_str, noise_str);

    ret = esp_http_client_set_header(client, "X-Temp", temp_str);
    if (ret != ESP_OK) goto cleanup;

    ret = esp_http_client_set_header(client, "X-Light", ldr_str);
    if (ret != ESP_OK) goto cleanup;

    ret = esp_http_client_set_header(client, "X-Noise", noise_str);
    if (ret != ESP_OK) goto cleanup;

    ret = esp_http_client_set_post_field(client, (const char *)audio_buffer, audio_bytes);
    if (ret != ESP_OK) goto cleanup;

    ESP_LOGI(TAG, "Sunucuya ses gonderiliyor...");
    ret = esp_http_client_perform(client);

    if (ret == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP POST tamam. Status=%d, CevapLen=%d", status_code, response_len);

        if (status_code == 200 && response_len > 0) {
            handle_server_response(response_buffer);
        }
    } else {
        ESP_LOGE(TAG, "HTTP hatasi: %s", esp_err_to_name(ret));
    }

cleanup:
    esp_http_client_cleanup(client);
    return ret;
}


// ---------------------------------------------------------------------------
// Ana Görev
// ---------------------------------------------------------------------------
static void jarvis_task(void *pvParameters)
{
    while (!wifi_connected) {
        ESP_LOGI(TAG, "Wi-Fi baglantisi bekleniyor...");
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    sensors_init();
    audio_init();

    int16_t *audio_buffer = malloc(AUDIO_BUFFER_SIZE);
    if (!audio_buffer) {
        ESP_LOGE(TAG, "audio_buffer ayirilamadi");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "============================");
    ESP_LOGI(TAG, "  JARVIS SISTEMI HAZIR");
    ESP_LOGI(TAG, "============================");

    while (1) {
        size_t audio_bytes = 0;

        // Sensorleri Oku
        read_sensors();

        if (pomodoro_state == POMODORO_RUNNING) {
            int mins = pomodoro_remaining_sec / 60;
            int secs = pomodoro_remaining_sec % 60;
            ESP_LOGI(TAG, "[Pomodoro aktif: %02d:%02d] Kayit aliniyor...", mins, secs);
        }

        if (record_audio_16bit(audio_buffer, &audio_bytes) == ESP_OK) {
            send_audio_to_server(audio_buffer, audio_bytes);
        }

        ESP_LOGI(TAG, "Yeni kayit icin 8 saniye bekleniyor...");
        vTaskDelay(pdMS_TO_TICKS(8000));
    }
}


void app_main(void)
{
    ESP_LOGI(TAG, "============================");
    ESP_LOGI(TAG, "  JARVIS BASLIYOR...");
    ESP_LOGI(TAG, "============================");

    wifi_init_sta();

    xTaskCreate(jarvis_task, "jarvis_task", 12288, NULL, 5, NULL);
}