#include "fan.h"
#include "config.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "fan";
static bool s_fan_on = false;

void fan_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << FAN_IN4_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(FAN_IN4_GPIO, 0); // Baslangicta kapali
    s_fan_on = false;
    ESP_LOGI(TAG, "Fan modulu hazir. GPIO %d (IN4), baslangiçta KAPALI.", FAN_IN4_GPIO);
}

void fan_on(void)
{
    if (!s_fan_on) {
        gpio_set_level(FAN_IN4_GPIO, 1);
        s_fan_on = true;
        ESP_LOGI(TAG, ">>> FAN ACILDI (GPIO %d = HIGH)", FAN_IN4_GPIO);
    } else {
        ESP_LOGI(TAG, "Fan zaten acik, islem atlandi.");
    }
}

void fan_off(void)
{
    if (s_fan_on) {
        gpio_set_level(FAN_IN4_GPIO, 0);
        s_fan_on = false;
        ESP_LOGI(TAG, ">>> FAN KAPATILDI (GPIO %d = LOW)", FAN_IN4_GPIO);
    } else {
        ESP_LOGI(TAG, "Fan zaten kapali, islem atlandi.");
    }
}

void fan_toggle(void)
{
    if (s_fan_on) {
        fan_off();
    } else {
        fan_on();
    }
}

bool fan_is_on(void)
{
    return s_fan_on;
}
