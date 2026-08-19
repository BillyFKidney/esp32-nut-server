/** @file nut-diagnostics.c @brief Hold bounded, RAM-only NUT disconnect simulation state. @see nut-diagnostics.h, esp_timer.h */
#include "nut-diagnostics.h"

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"

static portMUX_TYPE nut_diagnostics_lock = portMUX_INITIALIZER_UNLOCKED;
static int64_t nut_diagnostics_disconnect_until_us;

static bool nut_diagnostics_active_locked(int64_t now_us)
{
    if (nut_diagnostics_disconnect_until_us == 0)
    {
        return false;
    }
    if (now_us >= nut_diagnostics_disconnect_until_us)
    {
        nut_diagnostics_disconnect_until_us = 0;
        return false;
    }
    return true;
}

bool nut_diagnostics_start_disconnect_simulation(uint32_t duration_seconds)
{
    if (duration_seconds < NUT_DIAGNOSTIC_DISCONNECT_MIN_SECONDS ||
        duration_seconds > NUT_DIAGNOSTIC_DISCONNECT_MAX_SECONDS)
    {
        return false;
    }
    const int64_t now_us = esp_timer_get_time();
    portENTER_CRITICAL(&nut_diagnostics_lock);
    nut_diagnostics_disconnect_until_us =
        now_us + (int64_t)duration_seconds * 1000000LL;
    portEXIT_CRITICAL(&nut_diagnostics_lock);
    return true;
}

void nut_diagnostics_clear_disconnect_simulation(void)
{
    portENTER_CRITICAL(&nut_diagnostics_lock);
    nut_diagnostics_disconnect_until_us = 0;
    portEXIT_CRITICAL(&nut_diagnostics_lock);
}

bool nut_diagnostics_disconnect_simulation_active(void)
{
    const int64_t now_us = esp_timer_get_time();
    portENTER_CRITICAL(&nut_diagnostics_lock);
    const bool active = nut_diagnostics_active_locked(now_us);
    portEXIT_CRITICAL(&nut_diagnostics_lock);
    return active;
}
