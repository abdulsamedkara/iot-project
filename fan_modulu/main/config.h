#pragma once

// ─── Fan Motor (L298N) ──────────────────────────────────────────────────────
// IN4 pinini ESP32 GPIO 18'e bagla
// IN3 pinini GND'ye bagla (tek yon, sadece usfleme)
// L298N GND ile ESP32 GND mutlaka ayni noktaya baglanmali!

#define FAN_IN4_GPIO        18

// Otomatik mod: DS18B20 bu esigi asinca fan devreye girer
#define FAN_TEMP_THRESHOLD  28.0f   // Derece Celsius
