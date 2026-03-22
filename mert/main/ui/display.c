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
#include "ui_idle.h"
#include "ui_pomodoro.h"
#include "ui_sensor.h"
#include "ui_voice.h"
#include "ui_ai.h"

static const char *TAG = "DISPLAY";

static SemaphoreHandle_t lvgl_mux = NULL;
static lv_disp_t *disp;
static lv_disp_drv_t disp_drv;

lv_obj_t *scr_idle;
lv_obj_t *scr_pomodoro;
lv_obj_t *scr_sensor;
lv_obj_t *scr_voice;
lv_obj_t *scr_ai;

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io,
                                    esp_lcd_panel_io_event_data_t *edata,
                                    void *user_ctx)
{
    lv_disp_drv_t *drv = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(drv);
    return false;
}

static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;
    esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_map);
}

static void lvgl_tick_timer_cb(void *arg) { lv_tick_inc(2); }

static void display_task(void *arg) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
        if (display_lock(portMAX_DELAY)) {
            lv_timer_handler();
            display_unlock();
        }
    }
}

esp_err_t display_init(void) {
    lvgl_mux = xSemaphoreCreateMutex();

    spi_bus_config_t buscfg = {
        .sclk_io_num = TFT_SCLK_GPIO, .mosi_io_num = TFT_MOSI_GPIO,
        .miso_io_num = TFT_MISO_GPIO, .quadwp_io_num = -1, .quadhd_io_num = -1,
        .max_transfer_sz = TFT_WIDTH * 80 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(TFT_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = TFT_DC_GPIO, .cs_gpio_num = TFT_CS_GPIO,
        .pclk_hz = 20 * 1000 * 1000, .lcd_cmd_bits = 8, .lcd_param_bits = 8,
        .spi_mode = 0, .trans_queue_depth = 10,
        .on_color_trans_done = notify_lvgl_flush_ready, .user_ctx = &disp_drv,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)TFT_SPI_HOST, &io_config, &io_handle));

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = TFT_RST_GPIO, .rgb_endian = LCD_RGB_ENDIAN_BGR, .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    esp_lcd_panel_mirror(panel_handle, true, false);
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    lv_init();
    size_t draw_buf_size = TFT_WIDTH * 40;
    lv_color_t *buf_1 = heap_caps_malloc(draw_buf_size * sizeof(lv_color_t), MALLOC_CAP_DMA);
    lv_color_t *buf_2 = heap_caps_malloc(draw_buf_size * sizeof(lv_color_t), MALLOC_CAP_DMA);
    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, buf_1, buf_2, draw_buf_size);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = TFT_WIDTH; disp_drv.ver_res = TFT_HEIGHT;
    disp_drv.flush_cb = lvgl_flush_cb; disp_drv.draw_buf = &draw_buf;
    disp_drv.user_data = panel_handle;
    disp = lv_disp_drv_register(&disp_drv);

    const esp_timer_create_args_t t_args = { .callback = &lvgl_tick_timer_cb, .name = "lvgl_tick" };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&t_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 2 * 1000));

    // Tüm ekranları oluştur
    scr_idle     = lv_obj_create(NULL);
    scr_pomodoro = lv_obj_create(NULL);
    scr_sensor   = lv_obj_create(NULL);
    scr_voice    = lv_obj_create(NULL);
    scr_ai       = lv_obj_create(NULL);

    ui_idle_init(scr_idle);
    ui_pomodoro_init(scr_pomodoro);
    ui_sensor_init(scr_sensor);
    ui_voice_init(scr_voice);
    ui_ai_init(scr_ai);

    lv_disp_load_scr(scr_idle); // Varsayılan: Bekleme Ekranı

    xTaskCreate(display_task, "gui_task", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "Display OK.");
    return ESP_OK;
}

void display_switch_screen(screen_id_t id) {
    if (!display_lock(portMAX_DELAY)) return;
    switch (id) {
        case SCREEN_IDLE:     lv_disp_load_scr(scr_idle);     break;
        case SCREEN_POMODORO: lv_disp_load_scr(scr_pomodoro); break;
        case SCREEN_SENSOR:   lv_disp_load_scr(scr_sensor);   break;
        case SCREEN_VOICE:    lv_disp_load_scr(scr_voice);    break;
        case SCREEN_AI:       lv_disp_load_scr(scr_ai);       break;
    }
    display_unlock();
}

bool display_lock(int timeout_ms) {
    if (!lvgl_mux) return false;
    TickType_t t = (timeout_ms == portMAX_DELAY) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return (xSemaphoreTake(lvgl_mux, t) == pdTRUE);
}

void display_unlock(void) {
    if (lvgl_mux) xSemaphoreGive(lvgl_mux);
}
