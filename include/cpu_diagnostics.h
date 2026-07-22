#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    bool available;
    uint32_t utilization_percent;
    uint32_t sample_age_ms;
    uint32_t sample_interval_ms;
} CpuDiagnosticsSnapshot;

/** Start the low-overhead cached CPU-utilization sampler. */
void cpu_diagnostics_start(void);

/** Read the latest cached CPU-utilization sample without triggering a sample. */
void cpu_diagnostics_get_snapshot(CpuDiagnosticsSnapshot *snapshot);
