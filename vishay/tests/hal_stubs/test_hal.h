/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef VISHAY_TEST_HAL_H
#define VISHAY_TEST_HAL_H
#include <stdbool.h>
#include <stdint.h>
enum { GPIOA, GPIOB, GPIOC, GPIOD };
enum { ADC1, ADC2, ADC3 };
enum { PAL_MODE_INPUT, PAL_MODE_INPUT_PULLUP, PAL_MODE_INPUT_ANALOG,
       PAL_MODE_OUTPUT_PUSHPULL };
#define RCC_AHB1Periph_GPIOA 1U
#define RCC_AHB1Periph_GPIOB 2U
#define RCC_AHB1Periph_GPIOC 4U
#define RCC_AHB1Periph_GPIOD 8U
#define ENABLE 1
#define ADC_SampleTime_15Cycles 15
#define ADC_Channel_3 3
#define ADC_Channel_5 5
#define ADC_Channel_6 6
#define ADC_Channel_10 10
#define ADC_Channel_11 11
#define ADC_Channel_12 12
#define ADC_Channel_13 13
#define ADC_Channel_15 15
#define ADC_Channel_Vrefint 17
extern uint16_t ADC_Value[9];
void RCC_AHB1PeriphClockCmd(uint32_t clocks, int enable);
void palSetPadMode(unsigned port, unsigned pin, unsigned mode);
void palSetPad(unsigned port, unsigned pin);
void palClearPad(unsigned port, unsigned pin);
void palWritePad(unsigned port, unsigned pin, bool value);
bool palReadPad(unsigned port, unsigned pin);
void ADC_RegularChannelConfig(unsigned adc, unsigned channel, unsigned rank, unsigned sample_time);
void ADC_InjectedChannelConfig(unsigned adc, unsigned channel, unsigned rank, unsigned sample_time);
#endif
