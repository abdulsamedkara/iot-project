#include "pomodoro.h"
#include "esp_timer.h"
#include <string.h>

static pomo_state_t s_state = POMO_IDLE;
static int64_t s_state_start_us = 0;
static int s_completed_count = 0;

// Original Pomodoro duration (seconds)
// #define WORK_DURATION_SEC  (25 * 60)
// #define BREAK_DURATION_SEC (5 * 60)
// #define LONG_BREAK_SEC     (20 * 60)

// Uncomment here to drastically reduce durations for testing:
#define WORK_DURATION_SEC 30
#define BREAK_DURATION_SEC 20
#define LONG_BREAK_SEC 20

static char s_pending_event[32] = {0};

void pomodoro_init(void) {
    s_state = POMO_IDLE;
    s_completed_count = 0;
}

bool pomodoro_start(void) {
    if (s_state == POMO_WORK) {
        return false; // Already running
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
    if (s_state == POMO_LONG_BREAK) return "LONG_BREAK";
    return "IDLE";
}

int pomodoro_get_completed_count(void) {
    return s_completed_count;
}

uint32_t pomodoro_get_remaining_sec(void) {
    if (s_state == POMO_IDLE) return 0;
    
    int64_t now_us = esp_timer_get_time();
    int64_t elapsed_us = now_us - s_state_start_us;
    uint32_t elapsed_sec = elapsed_us / 1000000ULL;
    
    uint32_t total = pomodoro_get_total_sec();
    
    if (elapsed_sec >= total) return 0;
    return total - elapsed_sec;
}

uint32_t pomodoro_get_total_sec(void) {
    if (s_state == POMO_WORK) return WORK_DURATION_SEC;
    if (s_state == POMO_BREAK) return BREAK_DURATION_SEC;
    if (s_state == POMO_LONG_BREAK) return LONG_BREAK_SEC;
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
        s_completed_count++;
        if (s_completed_count % 4 == 0) {
            s_state = POMO_LONG_BREAK;
            strcpy(event_out, "POMO_WORK_DONE_FOR_LONG_BREAK");
        } else {
            s_state = POMO_BREAK;
            strcpy(event_out, "POMO_WORK_DONE");
        }
        s_state_start_us = now_us;
        return true;
    }

    if ((s_state == POMO_BREAK || s_state == POMO_LONG_BREAK) && 
        elapsed_sec >= pomodoro_get_total_sec()) {
        
        bool was_long = (s_state == POMO_LONG_BREAK);
        s_state = POMO_WORK; // Automatically start new work session
        s_state_start_us = now_us;
        
        if (was_long) {
            strcpy(event_out, "POMO_LONG_BREAK_DONE");
        } else {
            strcpy(event_out, "POMO_BREAK_DONE");
        }
        return true;
    }

    return false;
}
