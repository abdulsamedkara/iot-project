#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "display.h"
#include "ui_idle.h"
#include "ui_pomodoro.h"
#include "ui_sensor.h"
#include "ui_voice.h"
#include "ui_ai.h"

static const char *TAG = "MAIN";

// Demo dongusu adimlari
typedef enum {
    DEMO_IDLE,
    DEMO_POMODORO_WORK,
    DEMO_POMODORO_BREAK,
    DEMO_SENSOR_OK,
    DEMO_SENSOR_WARN,
    DEMO_VOICE_RECORD,
    DEMO_VOICE_PROCESS,
    DEMO_AI_THINK,
    DEMO_AI_ANSWER,
    DEMO_COUNT
} demo_step_t;

static demo_step_t s_step = DEMO_IDLE;
static int s_pomo_sec = 25 * 60;
static float s_temp = 22.5;
static int s_ldr = 2500;
static int s_completed = 3;

static void demo_tick(void) {
    if (!display_lock(200)) return;

    switch (s_step) {
        case DEMO_IDLE:
            lv_disp_load_scr(scr_idle);
            ui_idle_tick();
            break;

        case DEMO_POMODORO_WORK:
            lv_disp_load_scr(scr_pomodoro);
            s_pomo_sec -= 30;
            if (s_pomo_sec < 0) s_pomo_sec = 25 * 60;
            ui_pomodoro_update(POMO_WORK, s_pomo_sec, 25 * 60, s_completed);
            break;

        case DEMO_POMODORO_BREAK:
            lv_disp_load_scr(scr_pomodoro);
            ui_pomodoro_update(POMO_BREAK, 4 * 60, 5 * 60, s_completed);
            break;

        case DEMO_SENSOR_OK:
            lv_disp_load_scr(scr_sensor);
            s_temp = 22.5;
            s_ldr = 2500;
            ui_sensor_update(s_temp, s_ldr);
            break;

        case DEMO_SENSOR_WARN:
            lv_disp_load_scr(scr_sensor);
            s_temp = 31.0; // Sıcak uyarısı
            s_ldr = 200;   // Yetersiz ışık
            ui_sensor_update(s_temp, s_ldr);
            break;

        case DEMO_VOICE_RECORD:
            lv_disp_load_scr(scr_voice);
            ui_voice_set_state(VOICE_RECORDING);
            ui_voice_set_transcript("...");
            break;

        case DEMO_VOICE_PROCESS:
            lv_disp_load_scr(scr_voice);
            ui_voice_set_state(VOICE_PROCESSING);
            ui_voice_set_transcript("Pomodoro durdur");
            break;

        case DEMO_AI_THINK:
            lv_disp_load_scr(scr_ai);
            ui_ai_set_thinking(true);
            ui_ai_set_response("...", "Yapay Zeka");
            break;

        case DEMO_AI_ANSWER:
            lv_disp_load_scr(scr_ai);
            ui_ai_set_thinking(false);
            ui_ai_set_response(
                "Harika gidiyorsun! Bugun 3 pomodoro tamamladin. Kisa bir mola hak ettin.",
                "Yapay Zeka"
            );
            break;

        default:
            break;
    }

    display_unlock();

    s_step++;
    if (s_step >= DEMO_COUNT) s_step = DEMO_IDLE;
}

void app_main(void) {
    ESP_LOGI(TAG, "ESP32 Ekran Modülü başlatılıyor...");
    ESP_ERROR_CHECK(display_init());

    while (1) {
        demo_tick();
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
