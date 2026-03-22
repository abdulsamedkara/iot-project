#pragma once

// ─── WiFi ────────────────────────────────────────────────────────────────────
#define WIFI_SSID         "SUPERONLINE_Wi-Fi_A1C2"
#define WIFI_PASSWORD     "zKx2Z3uy7M"
#define WIFI_MAX_RETRY    10

// ─── PC Sunucu ───────────────────────────────────────────────────────────────
#define SERVER_HOST       "192.168.1.8"   // PC'nin yerel IP'sini girin
#define SERVER_PORT       8080

// ─── INMP441 I2S Mikrofon ────────────────────────────────────────────────────
#define MIC_I2S_PORT      I2S_NUM_0
#define MIC_SCK_GPIO      42
#define MIC_WS_GPIO       2
#define MIC_SD_GPIO       41
#define MIC_SAMPLE_RATE   16000

// ─── MAX98357A I2S Hoparlör ──────────────────────────────────────────────────
#define SPK_I2S_PORT      I2S_NUM_1
#define SPK_BCK_GPIO      16
#define SPK_WS_GPIO       17
#define SPK_DIN_GPIO      15

// Piper TTS varsayılan çıkış hızı (dfki-medium modeli için)
#define SPK_SAMPLE_RATE   22050

// ─── Buton ───────────────────────────────────────────────────────────────────
#define BUTTON_GPIO       4

// ─── Sensörler ───────────────────────────────────────────────────────────────
#define DS18B20_GPIO      8             // Önceden 9'du, 8 olarak değiştirildi.
#define LDR_ADC_CHANNEL   ADC_CHANNEL_4 // GPIO 5 için ADC_CHANNEL_4
#define SENSOR_UPDATE_MS  15000         // Her 15 saniyede bir kontrol et (kısaltıldı)
#define SMART_LED_GPIO    38            // Ortam karardıkça yanacak Akıllı LED Pini

// ─── Tampon Boyutları (PSRAM) ────────────────────────────────────────────────
// 16000 örnek/s × 2 bayt × 15 s = 480 000 bayt
#define MAX_RECORD_SECONDS  15
#define RECORD_BUF_SIZE     (MIC_SAMPLE_RATE * 2 * MAX_RECORD_SECONDS)

// 22050 × 2 × 45 s + WAV başlığı ~= 2 MB
#define RESP_BUF_SIZE       (SPK_SAMPLE_RATE * 2 * 45 + 1024)

// ─── TFT Ekran (ILI9341 & SPI) ────────────────────────────────────────────────
#define TFT_CS_GPIO       10
#define TFT_RST_GPIO      14
#define TFT_DC_GPIO       9
#define TFT_MOSI_GPIO     11
#define TFT_SCLK_GPIO     12
#define TFT_BCKL_GPIO     21
#define TFT_MISO_GPIO     -1 // Ekranda okuma yapmayacağız

#define TFT_WIDTH         240
#define TFT_HEIGHT        320
#define TFT_SPI_HOST      SPI2_HOST

