#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    POMO_IDLE = 0,
    POMO_WORK,
    POMO_BREAK
} pomo_state_t;

void pomodoro_init(void);
bool pomodoro_start(void);
void pomodoro_stop(void);
pomo_state_t pomodoro_get_state(void);
const char* pomodoro_get_state_str(void);
uint32_t pomodoro_get_remaining_sec(void);
uint32_t pomodoro_get_total_sec(void);

/* Her döngüde çağrılır, eğer süre dolduysa true ve event_out döndürür */
bool pomodoro_check_event(char* event_out);
