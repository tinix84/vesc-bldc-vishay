/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "raik060.h"

#include <stddef.h>

#define RAIK060_CRC_POLYNOMIAL 0x97U
#define RAIK060_CRC_BITS 8U
#define RAIK060_STATUS_BITS 2U
#define RAIK060_FIRST_CLOCK_DELAY_US 5U

uint8_t raik060_crc8(uint64_t data, uint8_t bit_count) {
    uint8_t crc = 0U;

    if (bit_count > 56U) {
        return 0U;
    }

    for (uint8_t remaining = bit_count; remaining > 0U; --remaining) {
        const uint8_t input_bit = (uint8_t)((data >> (remaining - 1U)) & 1U);
        const uint8_t feedback = (uint8_t)(((crc >> 7U) & 1U) ^ input_bit);

        crc = (uint8_t)(crc << 1U);
        if (feedback != 0U) {
            crc ^= RAIK060_CRC_POLYNOMIAL;
        }
    }

    return (uint8_t)~crc;
}

raik060_result_t raik060_decode(
    uint64_t raw,
    raik060_frame_kind_t kind,
    raik060_sample_t *sample) {
    uint8_t frame_bits;

    if (sample == NULL) {
        return RAIK060_BAD_ARGUMENT;
    }

    if (kind == RAIK060_FRAME_SINGLE_TURN) {
        frame_bits = RAIK060_SINGLE_TURN_FRAME_BITS;
    } else if (kind == RAIK060_FRAME_MULTI_TURN) {
        frame_bits = RAIK060_MULTI_TURN_FRAME_BITS;
    } else {
        return RAIK060_BAD_ARGUMENT;
    }

    if ((raw >> frame_bits) != 0U) {
        return RAIK060_BAD_ARGUMENT;
    }

    const uint64_t payload = raw >> RAIK060_CRC_BITS;
    const uint8_t payload_bits = (uint8_t)(frame_bits - RAIK060_CRC_BITS);
    const uint8_t status = (uint8_t)(payload & 0x3U);

    sample->raw = raw;
    sample->received_crc = (uint8_t)(raw & 0xFFU);
    sample->calculated_crc = raik060_crc8(payload, payload_bits);
    sample->warning = (status & 0x1U) == 0U;
    sample->error = (status & 0x2U) == 0U;
    sample->position = (uint32_t)((payload >> RAIK060_STATUS_BITS) &
                                  (RAIK060_POSITION_COUNTS - 1U));
    sample->turns = 0;

    if (kind == RAIK060_FRAME_MULTI_TURN) {
        sample->turns = (int16_t)(payload >>
                                  (RAIK060_POSITION_BITS + RAIK060_STATUS_BITS));
    }

    if (sample->received_crc != sample->calculated_crc) {
        return RAIK060_BAD_CRC;
    }

    if (sample->error) {
        return RAIK060_SENSOR_ERROR;
    }

    return RAIK060_OK;
}

float raik060_position_degrees(uint32_t position) {
    const uint32_t wrapped = position & (RAIK060_POSITION_COUNTS - 1U);
    return ((float)wrapped * 360.0F) / (float)RAIK060_POSITION_COUNTS;
}

raik060_result_t raik060_read(
    const raik060_bus_t *bus,
    raik060_frame_kind_t kind,
    raik060_sample_t *sample) {
    uint8_t frame_bits;
    uint64_t raw = 0U;

    if (bus == NULL || sample == NULL || bus->write_chip_select == NULL ||
        bus->write_clock == NULL || bus->read_data == NULL ||
        bus->delay_us == NULL || bus->half_period_us == 0U) {
        return RAIK060_BAD_ARGUMENT;
    }

    if (kind == RAIK060_FRAME_SINGLE_TURN) {
        frame_bits = RAIK060_SINGLE_TURN_FRAME_BITS;
    } else if (kind == RAIK060_FRAME_MULTI_TURN) {
        frame_bits = RAIK060_MULTI_TURN_FRAME_BITS;
    } else {
        return RAIK060_BAD_ARGUMENT;
    }

    bus->write_clock(bus->context, false);
    bus->write_chip_select(bus->context, false);
    bus->delay_us(bus->context, RAIK060_FIRST_CLOCK_DELAY_US);

    for (uint8_t bit = 0U; bit < frame_bits; ++bit) {
        /* Mode 1: the sensor shifts on the rising edge; sample after falling. */
        bus->write_clock(bus->context, true);
        bus->delay_us(bus->context, bus->half_period_us);
        bus->write_clock(bus->context, false);
        bus->delay_us(bus->context, bus->half_period_us);

        raw = (raw << 1U) | (bus->read_data(bus->context) ? 1U : 0U);
    }

    bus->write_chip_select(bus->context, true);
    return raik060_decode(raw, kind, sample);
}

