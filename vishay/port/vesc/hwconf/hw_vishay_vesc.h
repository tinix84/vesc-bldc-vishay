/* SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef HW_VISHAY_VESC_H_
#define HW_VISHAY_VESC_H_

/*
 * This target is intentionally non-buildable until the schematic pin export,
 * MCU part/package and analogue scaling values have been reviewed. Copying a
 * pin map from another VESC can destroy the power stage.
 */
#error "Vishay VESC hardware port pending reviewed board/pinmap.csv"

#define HW_NAME "Vishay VESC"
#define HW_MAJOR 0
#define HW_MINOR 1

/*
 * The completed file must define the full upstream VESC hardware contract:
 * PWM/timer channels, ADC sequence and indices, gate driver, current/voltage
 * scaling, temperatures, comms, halls and the chosen RAIK060 interface.
 * Start from third_party/bldc/hwconf/example/hw_example_core.h only after the
 * electrical values are known.
 */

#endif

