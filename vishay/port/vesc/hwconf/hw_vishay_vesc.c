/* SPDX-License-Identifier: GPL-3.0-or-later */
/* GPIO/ADC structure adapted from VESC hw_60_core.c.
 * Copyright 2012-2022 Benjamin Vedder <benjamin@vedder.se>.
 */

#include "hw.h"
#include "hal.h"
#include "timer.h"
#include <stddef.h>

void hw_vishay_early_init(void) {
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    DISABLE_GATE();
    palSetPadMode(HW_GATE_ENABLE_GPIO, HW_GATE_ENABLE_PIN, PAL_MODE_OUTPUT_PUSHPULL);
}

void hw_init_gpio(void) {
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB |
                          RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD, ENABLE);
    hw_vishay_early_init();

    LED_GREEN_OFF();
    LED_RED_OFF();
    palSetPadMode(GPIOB, 0, PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(GPIOB, 1, PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(GPIOB, 7, PAL_MODE_INPUT_PULLUP);

    /* DIN1/DIN2 are digital inputs, never phase-voltage ADC channels. */
    palSetPadMode(GPIOA, 0, PAL_MODE_INPUT);
    palSetPadMode(GPIOA, 1, PAL_MODE_INPUT);
    palSetPadMode(GPIOA, 2, PAL_MODE_INPUT);
    palSetPadMode(GPIOA, 3, PAL_MODE_INPUT_ANALOG);
    palSetPadMode(GPIOA, 5, PAL_MODE_INPUT_ANALOG);
    palSetPadMode(GPIOA, 6, PAL_MODE_INPUT_ANALOG);
    /* PA7 shares the COMM net with PB10. Keep PA7 high impedance. */
    palSetPadMode(GPIOA, 7, PAL_MODE_INPUT);
    palSetPadMode(GPIOB, 10, PAL_MODE_INPUT);
    palSetPadMode(GPIOB, 11, PAL_MODE_INPUT);

    for (unsigned pin = 0; pin <= 3; ++pin) {
        palSetPadMode(GPIOC, pin, PAL_MODE_INPUT_ANALOG);
    }
    /* PC5 is shutdown sense. Do not copy the VESC6 output/hold behavior. */
    palSetPadMode(GPIOC, 5, PAL_MODE_INPUT_ANALOG);
    CURRENT_FILTER_OFF();
    palSetPadMode(GPIOD, 2, PAL_MODE_OUTPUT_PUSHPULL);

    palSetPad(HW_RAIK060_CS_GPIO, HW_RAIK060_CS_PIN);
    palClearPad(HW_RAIK060_SCK_GPIO, HW_RAIK060_SCK_PIN);
    palSetPadMode(HW_RAIK060_CS_GPIO, HW_RAIK060_CS_PIN, PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(HW_RAIK060_SCK_GPIO, HW_RAIK060_SCK_PIN, PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(HW_RAIK060_MISO_GPIO, HW_RAIK060_MISO_PIN, PAL_MODE_INPUT);
    palSetPadMode(HW_RAIK060_MOSI_GPIO, HW_RAIK060_MOSI_PIN, PAL_MODE_INPUT);

    /* Keep driver SPI idle. Its OCP register settings are not yet approved. */
    palSetPad(DRV8301_CS_GPIO, DRV8301_CS_PIN);
    palClearPad(DRV8301_SCK_GPIO, DRV8301_SCK_PIN);
    palClearPad(DRV8301_MOSI_GPIO, DRV8301_MOSI_PIN);
    palSetPadMode(DRV8301_CS_GPIO, DRV8301_CS_PIN, PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(DRV8301_SCK_GPIO, DRV8301_SCK_PIN, PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(DRV8301_MOSI_GPIO, DRV8301_MOSI_PIN, PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(DRV8301_MISO_GPIO, DRV8301_MISO_PIN, PAL_MODE_INPUT);

    /* PWM pads stay inputs until dead time, polarity and gate protection pass review. */
    for (unsigned pin = 8; pin <= 10; ++pin) {
        palSetPadMode(GPIOA, pin, PAL_MODE_INPUT);
    }
    for (unsigned pin = 13; pin <= 15; ++pin) {
        palSetPadMode(GPIOB, pin, PAL_MODE_INPUT);
    }
}

void hw_setup_adc_channels(void) {
    const uint8_t sample_time = ADC_SampleTime_15Cycles;
    ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 1, sample_time);
    ADC_RegularChannelConfig(ADC2, ADC_Channel_11, 1, sample_time);
    ADC_RegularChannelConfig(ADC3, ADC_Channel_12, 1, sample_time);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_5, 2, sample_time);
    ADC_RegularChannelConfig(ADC2, ADC_Channel_6, 2, sample_time);
    ADC_RegularChannelConfig(ADC3, ADC_Channel_3, 2, sample_time);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_Vrefint, 3, sample_time);
    ADC_RegularChannelConfig(ADC2, ADC_Channel_15, 3, sample_time);
    ADC_RegularChannelConfig(ADC3, ADC_Channel_13, 3, sample_time);
    for (unsigned rank = 1; rank <= HW_ADC_INJ_CHANNELS; ++rank) {
        ADC_InjectedChannelConfig(ADC1, ADC_Channel_10, rank, sample_time);
        ADC_InjectedChannelConfig(ADC2, ADC_Channel_11, rank, sample_time);
        ADC_InjectedChannelConfig(ADC3, ADC_Channel_12, rank, sample_time);
    }
}

static void write_cs(void *context, bool high) {
    (void)context;
    palWritePad(HW_RAIK060_CS_GPIO, HW_RAIK060_CS_PIN, high);
}

static void write_clock(void *context, bool high) {
    (void)context;
    palWritePad(HW_RAIK060_SCK_GPIO, HW_RAIK060_SCK_PIN, high);
}

static bool read_data(void *context) {
    (void)context;
    return palReadPad(HW_RAIK060_MISO_GPIO, HW_RAIK060_MISO_PIN);
}

static void delay_us(void *context, uint32_t microseconds) {
    (void)context;
    timer_sleep(microseconds / 1000000.0f);
}

raik060_bus_t hw_vishay_raik060_bus(void) {
    return (raik060_bus_t){
        .context = NULL,
        .write_chip_select = write_cs,
        .write_clock = write_clock,
        .read_data = read_data,
        .delay_us = delay_us,
        .half_period_us = 1U,
    };
}

