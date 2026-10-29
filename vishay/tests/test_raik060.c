/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "raik060.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint64_t raw;
    uint8_t frame_bits;
    uint8_t read_index;
    unsigned clock_rises;
    unsigned chip_select_asserts;
    unsigned chip_select_deasserts;
    uint32_t first_delay_us;
    unsigned delay_calls;
} fake_bus_t;

static uint64_t make_frame(uint32_t position, int16_t turns, bool error, bool warning,
                           raik060_frame_kind_t kind) {
    const uint8_t status = (uint8_t)((error ? 0U : 2U) | (warning ? 0U : 1U));
    uint64_t payload = ((uint64_t)position << 2U) | status;
    uint8_t payload_bits = 20U;

    if (kind == RAIK060_FRAME_MULTI_TURN) {
        payload |= (uint64_t)(uint16_t)turns << 20U;
        payload_bits = 36U;
    }

    return (payload << 8U) | raik060_crc8(payload, payload_bits);
}

static void fake_write_chip_select(void *context, bool high) {
    fake_bus_t *fake = context;
    if (high) {
        ++fake->chip_select_deasserts;
    } else {
        ++fake->chip_select_asserts;
    }
}

static void fake_write_clock(void *context, bool high) {
    fake_bus_t *fake = context;
    if (high) {
        ++fake->clock_rises;
    }
}

static bool fake_read_data(void *context) {
    fake_bus_t *fake = context;
    const uint8_t shift = (uint8_t)(fake->frame_bits - 1U - fake->read_index);
    ++fake->read_index;
    return ((fake->raw >> shift) & 1U) != 0U;
}

static void fake_delay_us(void *context, uint32_t microseconds) {
    fake_bus_t *fake = context;
    if (fake->delay_calls == 0U) {
        fake->first_delay_us = microseconds;
    }
    ++fake->delay_calls;
}

static void test_crc_and_single_turn_decode(void) {
    raik060_sample_t sample = {0};

    assert(raik060_crc8(0U, 20U) == 0xFFU);

    const uint64_t frame = make_frame(0x2A55U, 0, false, false,
                                      RAIK060_FRAME_SINGLE_TURN);
    assert(raik060_decode(frame, RAIK060_FRAME_SINGLE_TURN, &sample) == RAIK060_OK);
    assert(sample.position == 0x2A55U);
    assert(!sample.error);
    assert(!sample.warning);
    assert(sample.received_crc == sample.calculated_crc);

    assert(raik060_decode(frame ^ 1U, RAIK060_FRAME_SINGLE_TURN, &sample) ==
           RAIK060_BAD_CRC);
}

static void test_status_and_multiturn_decode(void) {
    raik060_sample_t sample = {0};
    const uint64_t warning_frame = make_frame(42U, -7, false, true,
                                              RAIK060_FRAME_MULTI_TURN);

    assert(raik060_decode(warning_frame, RAIK060_FRAME_MULTI_TURN, &sample) ==
           RAIK060_OK);
    assert(sample.position == 42U);
    assert(sample.turns == -7);
    assert(!sample.error);
    assert(sample.warning);

    const uint64_t error_frame = make_frame(42U, 0, true, false,
                                            RAIK060_FRAME_SINGLE_TURN);
    assert(raik060_decode(error_frame, RAIK060_FRAME_SINGLE_TURN, &sample) ==
           RAIK060_SENSOR_ERROR);
    assert(sample.error);
}

static void test_angle_scale(void) {
    assert(fabsf(raik060_position_degrees(0U)) < 0.000001F);
    assert(fabsf(raik060_position_degrees(RAIK060_POSITION_COUNTS / 4U) - 90.0F) <
           0.000001F);
    assert(fabsf(raik060_position_degrees(RAIK060_POSITION_COUNTS - 1U) -
                 359.9986267F) < 0.0001F);
}

static void test_mode_1_transport(void) {
    fake_bus_t fake = {0};
    fake.frame_bits = RAIK060_SINGLE_TURN_FRAME_BITS;
    fake.raw = make_frame(123456U, 0, false, false, RAIK060_FRAME_SINGLE_TURN);

    const raik060_bus_t bus = {
        .context = &fake,
        .write_chip_select = fake_write_chip_select,
        .write_clock = fake_write_clock,
        .read_data = fake_read_data,
        .delay_us = fake_delay_us,
        .half_period_us = 1U,
    };
    raik060_sample_t sample = {0};

    assert(raik060_read(&bus, RAIK060_FRAME_SINGLE_TURN, &sample) == RAIK060_OK);
    assert(sample.position == 123456U);
    assert(fake.clock_rises == RAIK060_SINGLE_TURN_FRAME_BITS);
    assert(fake.read_index == RAIK060_SINGLE_TURN_FRAME_BITS);
    assert(fake.chip_select_asserts == 1U);
    assert(fake.chip_select_deasserts == 1U);
    assert(fake.first_delay_us >= 5U);
}

int main(void) {
    test_crc_and_single_turn_decode();
    test_status_and_multiturn_decode();
    test_angle_scale();
    test_mode_1_transport();
    puts("RAIK060 protocol tests passed");
    return 0;
}

