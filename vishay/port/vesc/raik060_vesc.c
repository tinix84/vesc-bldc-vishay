/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "raik060_vesc.h"

#include "ch.h"
#include "encoder/encoder.h"

#include <stdio.h>

#define RAIK060_VESC_THREAD_STACK 384U
#define RAIK060_DEFAULT_SAMPLE_PERIOD_US 100U
#define RAIK060_DEFAULT_STARTUP_DELAY_MS 160U
#define RAIK060_DEFAULT_ERROR_LIMIT 5U

static raik060_vesc_config_t active_config;
static volatile float last_angle_degrees;
static volatile uint32_t good_samples;
static volatile uint32_t bad_crc_samples;
static volatile uint32_t sensor_error_samples;
static volatile uint32_t consecutive_errors;
static volatile raik060_result_t last_result = RAIK060_BAD_ARGUMENT;
static bool started;

static THD_WORKING_AREA(raik060_thread_area, RAIK060_VESC_THREAD_STACK);

static float read_degrees(void) {
    return last_angle_degrees;
}

static bool has_fault(void) {
    return started &&
           consecutive_errors >= active_config.consecutive_errors_before_fault;
}

static char *print_info(void) {
    static char info[192];
    (void)snprintf(
        info,
        sizeof(info),
        "RAIK060: result=%d angle=%.3f good=%lu crc_errors=%lu sensor_errors=%lu consecutive=%lu",
        (int)last_result,
        (double)last_angle_degrees,
        (unsigned long)good_samples,
        (unsigned long)bad_crc_samples,
        (unsigned long)sensor_error_samples,
        (unsigned long)consecutive_errors);
    return info;
}

static THD_FUNCTION(raik060_thread, argument) {
    (void)argument;
    chRegSetThreadName("RAIK060");
    chThdSleepMilliseconds(active_config.startup_delay_ms);

    for (;;) {
        raik060_sample_t sample = {0};
        const raik060_result_t result = raik060_read(
            &active_config.bus, active_config.frame_kind, &sample);

        last_result = result;
        if (result == RAIK060_OK) {
            last_angle_degrees = raik060_position_degrees(sample.position);
            ++good_samples;
            consecutive_errors = 0U;
        } else {
            ++consecutive_errors;
            if (result == RAIK060_BAD_CRC) {
                ++bad_crc_samples;
            } else if (result == RAIK060_SENSOR_ERROR) {
                ++sensor_error_samples;
            }
        }

        chThdSleepMicroseconds(active_config.sample_period_us);
    }
}

bool raik060_vesc_start(const raik060_vesc_config_t *config) {
    if (config == NULL || started || config->bus.write_chip_select == NULL ||
        config->bus.write_clock == NULL || config->bus.read_data == NULL ||
        config->bus.delay_us == NULL) {
        return false;
    }

    active_config = *config;
    if (active_config.sample_period_us == 0U) {
        active_config.sample_period_us = RAIK060_DEFAULT_SAMPLE_PERIOD_US;
    }
    if (active_config.startup_delay_ms == 0U) {
        active_config.startup_delay_ms = RAIK060_DEFAULT_STARTUP_DELAY_MS;
    }
    if (active_config.consecutive_errors_before_fault == 0U) {
        active_config.consecutive_errors_before_fault = RAIK060_DEFAULT_ERROR_LIMIT;
    }

    encoder_set_custom_callbacks(read_degrees, has_fault, print_info);
    started = chThdCreateStatic(
                  raik060_thread_area,
                  sizeof(raik060_thread_area),
                  NORMALPRIO + 5,
                  raik060_thread,
                  NULL) != NULL;
    return started;
}

