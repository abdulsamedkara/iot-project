#include "display.h"
#include "config.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_ili9341.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "lvgl.h"
#include <stdio.h>

static const char *TAG = "DISPLAY";

static SemaphoreHandle_t lvgl_mux = NULL;

static lv_obj_t *lbl_status;
static lv_obj_t *lbl_timer;
static lv_disp_t *disp;
static lv_disp_drv_t disp_drv;

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

// LVGL flush callback
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
    // lv_disp_flush_ready() artık burda değil, SPI aktarımı bitince (notify_lvgl_flush_ready'de) çağrılacak.
}

// Tick timer callback
static void lvgl_tick_timer_cb(void *arg) {
    lv_tick_inc(2); // 2ms per tick
}

// LVGL Timer Task
static void display_task(void *arg) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
        if (lvgl_mux && xSemaphoreTake(lvgl_mux, portMAX_DELAY) == pdTRUE) {
            lv_timer_handler();
            xSemaphoreGive(lvgl_mux);
        }
    }
}

static void create_ui(void) {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_text_color(scr, lv_color_white(), 0);

    // Başlık
    lv_obj_t *lbl_title = lv_label_create(scr);
    lv_label_set_text(lbl_title, "Pomodoro Asistan");
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, 0); // 20 font yoksa 14

    // Sayaç (Timer)
    lbl_timer = lv_label_create(scr);
    lv_label_set_text(lbl_timer, "25:00");
    lv_obj_align(lbl_timer, LV_ALIGN_CENTER, 0, -10);
    // Büyük font seçmekte fayda var ama varsayılan varsa onu kullanıyoruz.

    // Durum (Status)
    lbl_status = lv_label_create(scr);
    lv_label_set_text(lbl_status, "Hazir");
    lv_obj_align(lbl_status, LV_ALIGN_BOTTOM_MID, 0, -20);
}

esp_err_t display_init(void) {
    lvgl_mux = xSemaphoreCreateMutex();
    
    ESP_LOGI(TAG, "Initializing SPI bus...");
    spi_bus_config_t buscfg = {
        .sclk_io_num = TFT_SCLK_GPIO,
        .mosi_io_num = TFT_MOSI_GPIO,
        .miso_io_num = TFT_MISO_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = TFT_WIDTH * 80 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(TFT_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Installing panel IO...");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = TFT_DC_GPIO,
        .cs_gpio_num = TFT_CS_GPIO,
        .pclk_hz = 20 * 1000 * 1000,  // Breadboard kablolari 40MHz'de gurultu/parlama yapabilir, 20MHz daha dengeli.
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = notify_lvgl_flush_ready,
        .user_ctx = &disp_drv,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)TFT_SPI_HOST, &io_config, &io_handle));

    ESP_LOGI(TAG, "Installing ILI9341 panel driver...");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = TFT_RST_GPIO,
        .rgb_endian = LCD_RGB_ENDIAN_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));
    
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    
    // Ayna goruntusunu (Mirroring) duzeltmek ucun:
    esp_lcd_panel_mirror(panel_handle, true, false);
    
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    
    // Arka plan aydınlatması
    gpio_set_direction(TFT_BCKL_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(TFT_BCKL_GPIO, 1);

    ESP_LOGI(TAG, "Initializing LVGL...");
    lv_init();
    
    size_t draw_buf_size = TFT_WIDTH * 40;
    lv_color_t *buf_1 = heap_caps_malloc(draw_buf_size * sizeof(lv_color_t), MALLOC_CAP_DMA);
    lv_color_t *buf_2 = heap_caps_malloc(draw_buf_size * sizeof(lv_color_t), MALLOC_CAP_DMA);
    
    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, buf_1, buf_2, draw_buf_size);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = TFT_WIDTH;
    disp_drv.ver_res = TFT_HEIGHT;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.user_data = panel_handle;
    disp = lv_disp_drv_register(&disp_drv);

    // Tick timer
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lvgl_tick_timer_cb,
        .name = "periodic_gui"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 2 * 1000));

    // Arayüz oluştur
    create_ui();

    // LVGL Task
    xTaskCreate(display_task, "gui_task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Display initialization completed.");
    return ESP_OK;
}

void display_update_status(const char* status_text) {
    if (lbl_status && lvgl_mux && xSemaphoreTake(lvgl_mux, portMAX_DELAY) == pdTRUE) {
        lv_label_set_text(lbl_status, status_text);
        xSemaphoreGive(lvgl_mux);
    }
}

void display_update_timer(int minutes, int seconds) {
    if (lbl_timer && lvgl_mux && xSemaphoreTake(lvgl_mux, portMAX_DELAY) == pdTRUE) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%02d:%02d", minutes, seconds);
        lv_label_set_text(lbl_timer, buf);
        xSemaphoreGive(lvgl_mux);
    }
}
