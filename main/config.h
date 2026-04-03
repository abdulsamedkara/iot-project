#pragma once

// ─── WiFi ────────────────────────────────────────────────────────────────────
#define WIFI_SSID         "YOUR_WIFI_SSID"
#define WIFI_PASSWORD     "YOUR_WIFI_PASSWORD"
#define WIFI_MAX_RETRY    10

// ─── PC Server ───────────────────────────────────────────────────────────────
#define SERVER_HOST       "YOUR_SERVER_IP"   // Enter PC's local IP
#define SERVER_PORT       8080

// ─── INMP441 I2S Microphone ──────────────────────────────────────────────────
#define MIC_I2S_PORT      I2S_NUM_0
#define MIC_SCK_GPIO      42
#define MIC_WS_GPIO       2
#define MIC_SD_GPIO       41
#define MIC_SAMPLE_RATE   16000

// ─── MAX98357A I2S Speaker ─────────────────────────────────────────────────────
#define SPK_I2S_PORT      I2S_NUM_1
#define SPK_BCK_GPIO      16
#define SPK_WS_GPIO       17
#define SPK_DIN_GPIO      15

// Piper TTS default output rate (for dfki-medium model)
#define SPK_SAMPLE_RATE   22050

// ─── Buttons ──────────────────────────────────────────────────────────────────
#define BUTTON_GPIO       4             // Main record button
#define LEFT_BUTTON_GPIO  0             // Left Screen Button (can also be Boot button)
#define RIGHT_BUTTON_GPIO 39            // Right Screen Button

// ─── Sensors ─────────────────────────────────────────────────────────────────
#define DS18B20_GPIO      8             // Formerly 9, changed to 8.
#define LDR_ADC_CHANNEL   ADC_CHANNEL_4 // ADC_CHANNEL_4 for GPIO 5
#define SENSOR_UPDATE_MS  15000         // Check every 15 seconds (shortened)
#define SMART_LED_GPIO    38            // Smart LED Pin (turns on as environment gets darker)

// ─── Buffer Sizes (PSRAM) ────────────────────────────────────────────────────
// 16000 samples/s × 2 bytes × 15 s = 480,000 bytes
#define MAX_RECORD_SECONDS  15
#define RECORD_BUF_SIZE     (MIC_SAMPLE_RATE * 2 * MAX_RECORD_SECONDS)

// 22050 × 2 × 45 s + WAV header ~= 2 MB
#define RESP_BUF_SIZE       (SPK_SAMPLE_RATE * 2 * 45 + 1024)

// ─── TFT Display (ILI9341 & SPI) ──────────────────────────────────────────────
#define TFT_CS_GPIO       10
#define TFT_RST_GPIO      14
#define TFT_DC_GPIO       9
#define TFT_MOSI_GPIO     11
#define TFT_SCLK_GPIO     12
#define TFT_BCKL_GPIO     21
#define TFT_MISO_GPIO     -1 // We won't read from the display

#define TFT_WIDTH         240
#define TFT_HEIGHT        320
#define TFT_SPI_HOST      SPI2_HOST

// ─── L298N Fan Motor Driver ───────────────────────────────────────────────────
// IN4 → When HIGH, motor spins (one direction)
#define FAN_IN4_GPIO      7             // L298N IN4 pin
#define FAN_ENA_GPIO      18            // L298N Enable A (PWM speed control)

// LEDC channel/timer (LEDC_CHANNEL_0/TIMER_0 is already used for LED)
#define FAN_LEDC_TIMER    LEDC_TIMER_1
#define FAN_LEDC_CHANNEL  LEDC_CHANNEL_1
#define FAN_PWM_FREQ_HZ   25000         // 25 kHz (silent frequency for DC motor)
#define FAN_DUTY_RES      LEDC_TIMER_8_BIT  // 0-255

// Temperature thresholds (°C)
#define FAN_TEMP_ON       28.0f         // Fan turns on above this temperature
#define FAN_TEMP_MAX      40.0f         // Fan runs at 100% speed at this temperature

