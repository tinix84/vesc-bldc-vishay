/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "hw.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

void hw_init_gpio(void);
void hw_setup_adc_channels(void);

uint16_t ADC_Value[9];
static uint32_t clocks_enabled;
static unsigned modes[4][16];
static bool levels[4][16];
static bool cleared[4][16];
static bool set_high[4][16];
static unsigned regular[3][3];
static unsigned injected[3][3];
static unsigned regular_count, injected_count;
static float delay_seconds;

void RCC_AHB1PeriphClockCmd(uint32_t clocks, int enable) {
    assert(enable == ENABLE);
    clocks_enabled |= clocks;
}

void palSetPadMode(unsigned port, unsigned pin, unsigned mode) {
    assert(clocks_enabled & (1U << port));
    if (port == GPIOB && pin == 5 && mode == PAL_MODE_OUTPUT_PUSHPULL) {
        assert(cleared[port][pin]);
        assert(!levels[port][pin]);
    }
    if (port == GPIOC && pin == 7 && mode == PAL_MODE_OUTPUT_PUSHPULL) {
        assert(set_high[port][pin]);
    }
    if (port == GPIOC && pin == 6 && mode == PAL_MODE_OUTPUT_PUSHPULL) {
        assert(cleared[port][pin]);
    }
    modes[port][pin] = mode;
}

void palWritePad(unsigned port, unsigned pin, bool value) {
    assert(clocks_enabled & (1U << port));
    assert(!(port == GPIOB && pin == 5 && value));
    levels[port][pin] = value;
    if (value) set_high[port][pin] = true;
    else cleared[port][pin] = true;
}

void palSetPad(unsigned port, unsigned pin) { palWritePad(port, pin, true); }
void palClearPad(unsigned port, unsigned pin) { palWritePad(port, pin, false); }
bool palReadPad(unsigned port, unsigned pin) { return levels[port][pin]; }
void timer_sleep(float seconds) { delay_seconds = seconds; }

void ADC_RegularChannelConfig(unsigned adc, unsigned channel, unsigned rank, unsigned sample_time) {
    assert(adc < 3 && rank > 0 && rank <= 3 && sample_time == ADC_SampleTime_15Cycles);
    assert(channel != 0 && channel != 1 && channel != 2 && channel != 14);
    regular[adc][rank - 1] = channel;
    ++regular_count;
}

void ADC_InjectedChannelConfig(unsigned adc, unsigned channel, unsigned rank, unsigned sample_time) {
    assert(adc < 3 && rank > 0 && rank <= 3 && sample_time == ADC_SampleTime_15Cycles);
    injected[adc][rank - 1] = channel;
    ++injected_count;
}

static void test_gpio_ownership(void) {
    memset(modes, 0xff, sizeof(modes));
    levels[GPIOB][5] = true; /* Must clear the latch before output mode. */
    hw_vishay_early_init();
    assert(!levels[GPIOB][5]);
    hw_init_gpio();
    ENABLE_GATE();
    assert(!levels[GPIOB][5]);
    assert(modes[GPIOB][5] == PAL_MODE_OUTPUT_PUSHPULL);
    assert(modes[GPIOB][7] == PAL_MODE_INPUT_PULLUP);
    assert(modes[GPIOA][0] == PAL_MODE_INPUT);
    assert(modes[GPIOA][1] == PAL_MODE_INPUT);
    assert(modes[GPIOA][7] == PAL_MODE_INPUT);
    assert(modes[GPIOB][10] == PAL_MODE_INPUT);
    assert(modes[GPIOC][4] == PAL_MODE_INPUT);
    assert(modes[GPIOC][5] == PAL_MODE_INPUT_ANALOG);
    assert(modes[GPIOC][8] == PAL_MODE_INPUT);
    assert(modes[GPIOB][2] == (unsigned)-1); /* Optional BMI270 stays untouched. */
    assert(modes[GPIOA][15] == (unsigned)-1);
    for (unsigned pin = 8; pin <= 10; ++pin) assert(modes[GPIOA][pin] == PAL_MODE_INPUT);
    for (unsigned pin = 13; pin <= 15; ++pin) assert(modes[GPIOB][pin] == PAL_MODE_INPUT);
    assert(levels[GPIOC][7] && !levels[GPIOC][6]);
    assert(levels[GPIOC][9] && !levels[GPIOC][10]);
    assert(!levels[GPIOD][2]);
}

static void test_adc_layout_and_scaling(void) {
    const unsigned expected_dma[] = {10, 11, 12, 5, 6, 3, 17, 15, 13};
    hw_setup_adc_channels();
    assert(regular_count == HW_ADC_CHANNELS && injected_count == 9);
    for (unsigned index = 0; index < HW_ADC_CHANNELS; ++index) {
        assert(regular[index % 3][index / 3] == expected_dma[index]);
    }
    assert(regular[ADC1][ADC_IND_CURR1 / 3] == 10);
    assert(regular[ADC2][ADC_IND_CURR2 / 3] == 11);
    assert(regular[ADC3][ADC_IND_CURR3 / 3] == 12);
    assert(regular[ADC_IND_TEMP_MOS % 3][ADC_IND_TEMP_MOS / 3] == 3);
    assert(regular[ADC_IND_VIN_SENS % 3][ADC_IND_VIN_SENS / 3] == 13);
    for (unsigned rank = 0; rank < 3; ++rank) {
        assert(injected[ADC1][rank] == 10);
        assert(injected[ADC2][rank] == 11);
        assert(injected[ADC3][rank] == 12);
    }
    assert(fabs(CURRENT_SHUNT_RES * CURRENT_AMP_GAIN - 0.04) < 1e-9);
    ADC_Value[ADC_IND_VIN_SENS] = 4095;
    assert(fabs(GET_INPUT_VOLTAGE() - 62.1) < 1e-6);
    assert(fabs(NTC_RES(2047.5) - 10000.0) < 1e-6);
}

static void test_encoder_bus_pins(void) {
    raik060_bus_t bus = hw_vishay_raik060_bus();
    assert(bus.half_period_us == 1);
    bus.write_chip_select(bus.context, false);
    assert(!levels[GPIOC][7]);
    bus.write_clock(bus.context, true);
    assert(levels[GPIOC][6]);
    levels[GPIOC][4] = true;
    assert(bus.read_data(bus.context));
    levels[GPIOC][4] = false;
    assert(!bus.read_data(bus.context));
    bus.delay_us(bus.context, 5);
    assert(fabsf(delay_seconds - 0.000005f) < 1e-10f);
    bus.write_chip_select(bus.context, true);
    bus.write_clock(bus.context, false);
}

int main(void) {
    test_gpio_ownership();
    test_adc_layout_and_scaling();
    test_encoder_bus_pins();
    puts("Vishay GPIO ownership, ADC ordering, nominal scaling and SPI bus tests passed");
    return 0;
}
