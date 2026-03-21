#include "pomodoro.h"
#include "esp_timer.h"
#include <string.h>

static pomo_state_t s_state = POMO_IDLE;
static int64_t s_state_start_us = 0;

// Süreler (saniye). Gerçek kullanım için: 25*60 ve 5*60
#define WORK_DURATION_SEC (2 * 60)
#define BREAK_DURATION_SEC (1 * 60)

// Test için süreleri çok düşürmek istersen:
// #define WORK_DURATION_SEC 10
// #define BREAK_DURATION_SEC 5

static char s_pending_event[32] = {0};

void pomodoro_init(void) {
    s_state = POMO_IDLE;
}

bool pomodoro_start(void) {
    if (s_state == POMO_WORK) {
        return false; // Zaten çalışıyor
    }
    s_state = POMO_WORK;
    s_state_start_us = esp_timer_get_time();
    return true;
}

void pomodoro_stop(void) {
    s_state = POMO_IDLE;
    s_pending_event[0] = '\0';
}

pomo_state_t pomodoro_get_state(void) {
    return s_state;
}

const char* pomodoro_get_state_str(void) {
    if (s_state == POMO_WORK) return "WORK";
    if (s_state == POMO_BREAK) return "BREAK";
    return "IDLE";
}

uint32_t pomodoro_get_remaining_sec(void) {
    if (s_state == POMO_IDLE) return 0;
    
    int64_t now_us = esp_timer_get_time();
    int64_t elapsed_us = now_us - s_state_start_us;
    uint32_t elapsed_sec = elapsed_us / 1000000ULL;
    
    uint32_t total = (s_state == POMO_WORK) ? WORK_DURATION_SEC : BREAK_DURATION_SEC;
    
    if (elapsed_sec >= total) return 0;
    return total - elapsed_sec;
}

uint32_t pomodoro_get_total_sec(void) {
    if (s_state == POMO_WORK) return WORK_DURATION_SEC;
    if (s_state == POMO_BREAK) return BREAK_DURATION_SEC;
    return 0;
}

bool pomodoro_check_event(char* event_out) {
    if (s_pending_event[0] != '\0') {
        strcpy(event_out, s_pending_event);
        s_pending_event[0] = '\0';
        return true;
    }

    if (s_state == POMO_IDLE) return false;

    int64_t now_us = esp_timer_get_time();
    int64_t elapsed_us = now_us - s_state_start_us;
    uint32_t elapsed_sec = elapsed_us / 1000000ULL;

    if (s_state == POMO_WORK && elapsed_sec >= WORK_DURATION_SEC) {
        s_state = POMO_BREAK;
        s_state_start_us = now_us;
        strcpy(event_out, "POMO_WORK_DONE");
        return true;
    }

    if (s_state == POMO_BREAK && elapsed_sec >= BREAK_DURATION_SEC) {
        s_state = POMO_WORK;
        s_state_start_us = now_us;
        strcpy(event_out, "POMO_BREAK_DONE");
        return true;
    }

    return false;
}
