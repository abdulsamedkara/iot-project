#pragma once

// ─── TFT Ekran (ILI9341 & SPI) ────────────────────────────────────────────────
#define TFT_CS_GPIO       10
#define TFT_RST_GPIO      11
#define TFT_DC_GPIO       12
#define TFT_MOSI_GPIO     13
#define TFT_SCLK_GPIO     14
#define TFT_BCKL_GPIO     -1  // Sabit 3.3V
#define TFT_MISO_GPIO     -1  // Kullanilmiyor

#define TFT_WIDTH         240
#define TFT_HEIGHT        320
#define TFT_SPI_HOST      SPI2_HOST
