idf_component_register(
    SRCS
        "main.c"
        "pomodoro.c"
        "i2s_mic.c"
        "i2s_player.c"
        "voice_client.c"
        "display.c"
    INCLUDE_DIRS "."
    REQUIRES
        esp_wifi
        nvs_flash
        esp_http_client
        driver
        esp_event
        esp_timer
        esp_adc
)