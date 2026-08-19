#pragma once

#include <stdbool.h>
#include <stdint.h>

#define NUT_DIAGNOSTIC_DISCONNECT_MIN_SECONDS 1U
#define NUT_DIAGNOSTIC_DISCONNECT_MAX_SECONDS 300U

/** Enable a bounded, RAM-only simulated UPS disconnect. */
bool nut_diagnostics_start_disconnect_simulation(uint32_t duration_seconds);

/** Clear a simulated UPS disconnect early. Safe when already inactive. */
void nut_diagnostics_clear_disconnect_simulation(void);

/** Return whether the simulated disconnect remains active. */
bool nut_diagnostics_disconnect_simulation_active(void);
