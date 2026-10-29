/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Portable RAIK060 SPI framing and bit-banged transport.
 */

#ifndef RAIK060_H
#define RAIK060_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RAIK060_POSITION_BITS 18U
#define RAIK060_SINGLE_TURN_FRAME_BITS 28U
#define RAIK060_MULTI_TURN_FRAME_BITS 44U
#define RAIK060_POSITION_COUNTS (1UL << RAIK060_POSITION_BITS)

typedef enum {
    RAIK060_FRAME_SINGLE_TURN = 0,
    RAIK060_FRAME_MULTI_TURN = 1
} raik060_frame_kind_t;

typedef enum {
    RAIK060_OK = 0,
    RAIK060_BAD_ARGUMENT,
    RAIK060_BAD_CRC,
    RAIK060_SENSOR_ERROR
} raik060_result_t;

typedef struct {
    uint32_t position;
    int16_t turns;
    bool error;
    bool warning;
    uint8_t received_crc;
    uint8_t calculated_crc;
    uint64_t raw;
} raik060_sample_t;

typedef void (*raik060_write_pin_fn)(void *context, bool high);
typedef bool (*raik060_read_pin_fn)(void *context);
typedef void (*raik060_delay_us_fn)(void *context, uint32_t microseconds);

typedef struct {
    void *context;
    raik060_write_pin_fn write_chip_select;
    raik060_write_pin_fn write_clock;
    raik060_read_pin_fn read_data;
    raik060_delay_us_fn delay_us;
    uint32_t half_period_us;
} raik060_bus_t;

/* CRC-8 over an MSB-first bit string, polynomial 0x97, seed 0, inverted output. */
uint8_t raik060_crc8(uint64_t data, uint8_t bit_count);

/* Decode the low 28 or 44 bits of raw according to kind. */
raik060_result_t raik060_decode(
    uint64_t raw,
    raik060_frame_kind_t kind,
    raik060_sample_t *sample);

float raik060_position_degrees(uint32_t position);

/*
 * Read a simple, read-only frame using SPI mode 1 timing.
 * MOSI is intentionally absent: the RAIK060 datasheet makes it optional for
 * this frame. Register access and sensor commands need a separate MOSI path.
 */
raik060_result_t raik060_read(
    const raik060_bus_t *bus,
    raik060_frame_kind_t kind,
    raik060_sample_t *sample);

#ifdef __cplusplus
}
#endif

#endif

