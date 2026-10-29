/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef HW_VISHAY_CORE_H_
#define HW_VISHAY_CORE_H_

#include "../../../include/raik060.h"

/* Provisional assignments from the MDRV1P3VESC_VAR1 board review. */
#define HW_NAME "Vishay VESC provisional"
#define HW_MAJOR 0
#define HW_MINOR 1
#define STM32_HSECLK 8000000U
#define HW_HAS_DRV8301
#define HW_HAS_3_SHUNTS
#define HW_HAS_PHASE_SHUNTS

#define HW_GATE_ENABLE_GPIO GPIOB
#define HW_GATE_ENABLE_PIN 5
#define DISABLE_GATE() palClearPad(HW_GATE_ENABLE_GPIO, HW_GATE_ENABLE_PIN)
/* Gate activation is unavailable until the board and protection are reviewed. */
#define ENABLE_GATE() DISABLE_GATE()
#define IS_DRV_FAULT() (!palReadPad(GPIOB, 7))
#define HW_EARLY_INIT() hw_vishay_early_init()

#define LED_GREEN_ON() palSetPad(GPIOB, 0)
#define LED_GREEN_OFF() palClearPad(GPIOB, 0)
#define LED_RED_ON() palSetPad(GPIOB, 1)
#define LED_RED_OFF() palClearPad(GPIOB, 1)
#define CURRENT_FILTER_ON() palSetPad(GPIOD, 2)
#define CURRENT_FILTER_OFF() palClearPad(GPIOD, 2)

#define DRV8301_SCK_GPIO GPIOC
#define DRV8301_SCK_PIN 10
#define DRV8301_MISO_GPIO GPIOB
#define DRV8301_MISO_PIN 3
#define DRV8301_MOSI_GPIO GPIOB
#define DRV8301_MOSI_PIN 4
#define DRV8301_CS_GPIO GPIOC
#define DRV8301_CS_PIN 9

#define HW_RAIK060_SCK_GPIO GPIOC
#define HW_RAIK060_SCK_PIN 6
#define HW_RAIK060_CS_GPIO GPIOC
#define HW_RAIK060_CS_PIN 7
#define HW_RAIK060_MOSI_GPIO GPIOC
#define HW_RAIK060_MOSI_PIN 8
#define HW_RAIK060_MISO_GPIO GPIOC
#define HW_RAIK060_MISO_PIN 4

/* No hall, phase-voltage or motor-temperature aliases exist on this board. */
#define HW_ADC_INJ_CHANNELS 3
#define HW_ADC_NBR_CONV 3
#define HW_ADC_CHANNELS (HW_ADC_NBR_CONV * 3)
/* Triple-ADC DMA order: ADC1, ADC2, ADC3 for each rank. */
#define ADC_IND_CURR1 0
#define ADC_IND_CURR2 1
#define ADC_IND_CURR3 2
#define ADC_IND_EXT 3
#define ADC_IND_EXT2 4
#define ADC_IND_TEMP_MOS 5
#define ADC_IND_VREFINT 6
#define ADC_IND_SHUTDOWN 7
#define ADC_IND_VIN_SENS 8

#define V_REG 3.3
#define VIN_R1 39200.0
#define VIN_R2 2200.0
#define CURRENT_AMP_GAIN 20.0
#define CURRENT_SHUNT_RES 0.002
/* Nominal only: Q1 transfer and ADC reference require measurement. */
#define GET_INPUT_VOLTAGE() ((V_REG / 4095.0) * (float)ADC_Value[ADC_IND_VIN_SENS] * ((VIN_R1 + VIN_R2) / VIN_R2))
#define ADC_VOLTS(ch) ((float)ADC_Value[ch] / 4096.0 * V_REG)
/* RT3 is high-side 10k; no temperature conversion without its actual curve. */
#define NTC_RES(adc_val) ((4095.0 * 10000.0) / (adc_val) - 10000.0)

#define HW_ADC_EXT_GPIO GPIOA
#define HW_ADC_EXT_PIN 5
#define HW_ADC_EXT2_GPIO GPIOA
#define HW_ADC_EXT2_PIN 6
#define HW_UART_DEV SD3
#define HW_UART_GPIO_AF GPIO_AF_USART3
#define HW_UART_TX_PORT GPIOB
#define HW_UART_TX_PIN 10
#define HW_UART_RX_PORT GPIOB
#define HW_UART_RX_PIN 11

/* BMI270 PB2=SCL / PA15=SDA remains disabled; no BMI160 alias. */
void hw_vishay_early_init(void);
raik060_bus_t hw_vishay_raik060_bus(void);

#endif
