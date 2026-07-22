#include "cpu_diagnostics.h"

#include <limits.h>
#include <string.h>

#include "esp_attr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "sdkconfig.h"

#define TAG "cpu-diagnostics"
#define CPU_DIAGNOSTICS_SAMPLE_INTERVAL_MS 10000U
#define CPU_DIAGNOSTICS_TASK_STACK_SIZE 3072U
#define CPU_DIAGNOSTICS_TASK_PRIORITY (tskIDLE_PRIORITY + 1U)
#define CPU_DIAGNOSTICS_CORE_COUNT CONFIG_FREERTOS_NUMBER_OF_CORES

typedef struct
{
    volatile uint32_t tick_count;
    volatile uint32_t idle_tick_count;
    volatile bool idle_seen;
} CpuDiagnosticsCoreCounters;

static CpuDiagnosticsCoreCounters cpu_diagnostics_core_counters[
    CPU_DIAGNOSTICS_CORE_COUNT];
static portMUX_TYPE cpu_diagnostics_snapshot_lock = portMUX_INITIALIZER_UNLOCKED;
static bool cpu_diagnostics_started;
static bool cpu_diagnostics_available;
static uint32_t cpu_diagnostics_utilization_percent;
static uint32_t cpu_diagnostics_sample_interval_ms =
    CPU_DIAGNOSTICS_SAMPLE_INTERVAL_MS;
static uint64_t cpu_diagnostics_sampled_at_ms;

/* These callbacks run from FreeRTOS idle/tick hooks while flash cache may be disabled. */
static bool IRAM_ATTR cpu_diagnostics_idle_hook(void)
{
    const UBaseType_t core = xPortGetCoreID();
    if (core < CPU_DIAGNOSTICS_CORE_COUNT)
    {
        cpu_diagnostics_core_counters[core].idle_seen = true;
    }
    return true;
}

static void IRAM_ATTR cpu_diagnostics_tick_hook(void)
{
    const UBaseType_t core = xPortGetCoreID();
    if (core >= CPU_DIAGNOSTICS_CORE_COUNT)
    {
        return;
    }

    CpuDiagnosticsCoreCounters *counters = &cpu_diagnostics_core_counters[core];
    counters->tick_count++;
    if (counters->idle_seen)
    {
        counters->idle_tick_count++;
        counters->idle_seen = false;
    }
}

static void cpu_diagnostics_unregister_hooks(void)
{
    for (UBaseType_t core = 0; core < CPU_DIAGNOSTICS_CORE_COUNT; core++)
    {
        esp_deregister_freertos_idle_hook_for_cpu(cpu_diagnostics_idle_hook, core);
        esp_deregister_freertos_tick_hook_for_cpu(cpu_diagnostics_tick_hook, core);
    }
}

static bool cpu_diagnostics_register_hooks(void)
{
    for (UBaseType_t core = 0; core < CPU_DIAGNOSTICS_CORE_COUNT; core++)
    {
        if (esp_register_freertos_idle_hook_for_cpu(
                cpu_diagnostics_idle_hook, core) != ESP_OK ||
            esp_register_freertos_tick_hook_for_cpu(
                cpu_diagnostics_tick_hook, core) != ESP_OK)
        {
            cpu_diagnostics_unregister_hooks();
            return false;
        }
    }
    return true;
}

static void cpu_diagnostics_sampler_task(void *argument)
{
    (void)argument;

    uint32_t previous_tick_counts[CPU_DIAGNOSTICS_CORE_COUNT] = {0};
    uint32_t previous_idle_tick_counts[CPU_DIAGNOSTICS_CORE_COUNT] = {0};
    for (UBaseType_t core = 0; core < CPU_DIAGNOSTICS_CORE_COUNT; core++)
    {
        previous_tick_counts[core] =
            cpu_diagnostics_core_counters[core].tick_count;
        previous_idle_tick_counts[core] =
            cpu_diagnostics_core_counters[core].idle_tick_count;
    }
    uint64_t previous_sample_at_ms = (uint64_t)(esp_timer_get_time() / 1000LL);

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(CPU_DIAGNOSTICS_SAMPLE_INTERVAL_MS));

        uint32_t current_tick_counts[CPU_DIAGNOSTICS_CORE_COUNT];
        uint32_t current_idle_tick_counts[CPU_DIAGNOSTICS_CORE_COUNT];
        uint64_t total_ticks = 0;
        uint64_t idle_ticks = 0;
        for (UBaseType_t core = 0; core < CPU_DIAGNOSTICS_CORE_COUNT; core++)
        {
            current_tick_counts[core] =
                cpu_diagnostics_core_counters[core].tick_count;
            current_idle_tick_counts[core] =
                cpu_diagnostics_core_counters[core].idle_tick_count;
            total_ticks += (uint32_t)(current_tick_counts[core] -
                                      previous_tick_counts[core]);
            idle_ticks += (uint32_t)(current_idle_tick_counts[core] -
                                     previous_idle_tick_counts[core]);
        }

        const uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000LL);
        const uint64_t sample_interval_ms = now_ms - previous_sample_at_ms;
        if (idle_ticks > total_ticks)
        {
            idle_ticks = total_ticks;
        }

        if (total_ticks > 0 && sample_interval_ms > 0)
        {
            const uint64_t busy_ticks = total_ticks - idle_ticks;
            uint64_t utilization_percent =
                (busy_ticks * 100U + (total_ticks / 2U)) / total_ticks;
            if (utilization_percent > 100U)
            {
                utilization_percent = 100U;
            }

            taskENTER_CRITICAL(&cpu_diagnostics_snapshot_lock);
            cpu_diagnostics_available = true;
            cpu_diagnostics_utilization_percent =
                (uint32_t)utilization_percent;
            cpu_diagnostics_sample_interval_ms = sample_interval_ms > UINT32_MAX
                                                    ? UINT32_MAX
                                                    : (uint32_t)sample_interval_ms;
            cpu_diagnostics_sampled_at_ms = now_ms;
            taskEXIT_CRITICAL(&cpu_diagnostics_snapshot_lock);
        }

        for (UBaseType_t core = 0; core < CPU_DIAGNOSTICS_CORE_COUNT; core++)
        {
            previous_tick_counts[core] = current_tick_counts[core];
            previous_idle_tick_counts[core] = current_idle_tick_counts[core];
        }
        previous_sample_at_ms = now_ms;
    }
}

void cpu_diagnostics_start(void)
{
    if (cpu_diagnostics_started)
    {
        return;
    }
    cpu_diagnostics_started = true;

    if (!cpu_diagnostics_register_hooks())
    {
        ESP_LOGW(TAG, "CPU utilization sampling hooks are unavailable");
        return;
    }

    if (xTaskCreate(cpu_diagnostics_sampler_task, "cpu-sampler",
                    CPU_DIAGNOSTICS_TASK_STACK_SIZE, NULL,
                    CPU_DIAGNOSTICS_TASK_PRIORITY, NULL) != pdPASS)
    {
        cpu_diagnostics_unregister_hooks();
        ESP_LOGW(TAG, "Unable to create CPU utilization sampler");
    }
}

void cpu_diagnostics_get_snapshot(CpuDiagnosticsSnapshot *snapshot)
{
    if (snapshot == NULL)
    {
        return;
    }

    memset(snapshot, 0, sizeof(*snapshot));
    uint64_t sampled_at_ms = 0;
    taskENTER_CRITICAL(&cpu_diagnostics_snapshot_lock);
    snapshot->available = cpu_diagnostics_available;
    snapshot->utilization_percent = cpu_diagnostics_utilization_percent;
    snapshot->sample_interval_ms = cpu_diagnostics_sample_interval_ms;
    sampled_at_ms = cpu_diagnostics_sampled_at_ms;
    taskEXIT_CRITICAL(&cpu_diagnostics_snapshot_lock);

    if (snapshot->available)
    {
        const uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000LL);
        if (now_ms >= sampled_at_ms)
        {
            const uint64_t age_ms = now_ms - sampled_at_ms;
            snapshot->sample_age_ms = age_ms > UINT32_MAX
                                          ? UINT32_MAX
                                          : (uint32_t)age_ms;
        }
    }
}
