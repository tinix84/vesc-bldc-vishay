/* SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef RAIK060_VESC_H
#define RAIK060_VESC_H

#include "../../include/raik060.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    raik060_bus_t bus;
    raik060_frame_kind_t frame_kind;
    uint32_t sample_period_us;
    uint32_t startup_delay_ms;
    uint32_t consecutive_errors_before_fault;
} raik060_vesc_config_t;

/*
 * Register the RAIK060 as VESC's SENSOR_PORT_MODE_CUSTOM_ENCODER and start
 * the sampling thread. The GPIO context referenced by config must remain
 * valid for the lifetime of the firmware.
 */
bool raik060_vesc_start(const raik060_vesc_config_t *config);

#endif

