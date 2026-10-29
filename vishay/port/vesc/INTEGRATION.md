# Vishay hardware integration

Status: **provisional implementation; motor firmware build blocked**.
The user confirmed on 2026-10-29 that the reviewed board has no newer revision.
The firmware baseline is VESC commit `4b4dd3062dcbb3dbdd188b8dd2d3e6b88ad69da8`.

## Implemented configuration

`hwconf/hw_vishay_core.h` contains the supported pin and nominal scaling definitions.
`hwconf/hw_vishay_vesc.c` implements GPIO initialization, ADC channel setup, and the RAIK060 GPIO SPI bus callbacks.

- STM32F405RGT7TR, LQFP64, with an 8 MHz crystal.
- Gate enable PB5 stays low before and after GPIO initialization.
- Gate fault uses PB7. No independent timer-break connection is established.
- Driver SPI uses PC10 clock, PB3 MISO, PB4 MOSI, and PC9 chip select.
- Encoder SPI uses PC6 clock, PC7 chip select, PC8 MOSI, and PC4 MISO.
- Three current inputs use PC0/PC1/PC2 and ADC channels 10/11/12.
- Nominal current scaling uses 2 mOhm shunts and gain-20 AD8418A amplifiers.
- Nominal bus scaling uses R16=39.2k and R20=2.2k. Q1 requires transfer calibration.
- PA3 samples the PCB NTC. Its resistance conversion uses the reviewed high-side divider.
- PA0/PA1 remain digital inputs. PC4 remains encoder MISO.
- PA7 stays an input because it shares a COMM net with PB10.
- PC5 remains a shutdown-sense ADC input. Supply hold/release control is unverified.
- BMI270, independent motor halls, and motor-temperature conversion remain unavailable.

All GPIO assignments remain provisional until source connectivity and schematic/PCB parity are resolved.
The worksheet includes MCU pad numbers, PCB net names, and verification status for each assigned GPIO.
No source CAD export accompanies the public firmware support.

## ADC scan

The regular scan has three ranks across three ADCs.
Each rank enters DMA memory in ADC1, ADC2, ADC3 order.

| Rank | ADC1 | ADC2 | ADC3 | DMA indices |
|---|---|---|---|---|
| 1 | PC0 current A, channel 10 | PC1 current B, channel 11 | PC2 current C, channel 12 | 0, 1, 2 |
| 2 | PA5 external input, channel 5 | PA6 external input, channel 6 | PA3 PCB NTC, channel 3 | 3, 4, 5 |
| 3 | Vrefint, channel 17 | PC5 shutdown, channel 15 | PC3 bus voltage, channel 13 | 6, 7, 8 |

All three injected ranks sample current channels 10/11/12 through ADC1/ADC2/ADC3.
This provisional layout excludes channels 0/1/2 and 14.
It does not provide phase-voltage or motor-temperature measurements.
The hardware build remains blocked because the VESC motor code requires additional measurements and configuration.

## Build guard and disabled functions

`hw_vishay_vesc.h` contains an unconditional build error.
There is no build flag that approves the unresolved hardware.
`ENABLE_GATE()` also holds PB5 low, and initialization keeps all six PWM pads as inputs.
Host tests compile the GPIO/ADC implementation through a simulated HAL boundary.
They do not compile or validate a complete motor firmware image.

The implementation does not call `drv8301_init()`.
The pinned implementation writes an overcurrent-disable configuration during initialization.
The driver protection sequence needs review before gate activation or register initialization.
The code does not copy VESC6 shutdown control, NTC beta, phase ADC inputs, or motor-temperature aliases.

## Remaining requirements

1. Establish phase-voltage sensing or review a complete motor-control alternative.
2. Restore and verify the VDDA supply feed.
3. Resolve the schematic/PCB connectivity differences against the original Altium design.
4. Confirm the RAIK060 ordering code and single-turn or multi-turn frame.
5. Specify the PCB NTC curve and motor-temperature strategy.
6. Approve voltage, current, temperature, regeneration, and gate-protection limits.
7. Verify current polarity, ADC reference, bus transfer, and filter settling on hardware.
8. Confirm physical SWD access, shutdown behavior, and COMM peripheral ownership.

Do not substitute digital input values or constants for missing phase-voltage measurements.
Do not infer permitted board current from the amplifier rail range.

## Encoder integration after review

The existing board has GPIO SPI wiring and no verified BiSS-C differential interface.
`hw_vishay_raik060_bus()` returns the board-specific GPIO callbacks with a 1 us half-period.
Its delay callback uses the pinned VESC `timer_sleep()` API.
The bus reads PC4 and leaves PC8 high impedance for position-only transfers.

After confirming the sensor frame, pass this bus to `raik060_vesc_start()` through a `raik060_vesc_config_t`.
Add `src/raik060.c` and `port/vesc/raik060_vesc.c` to the firmware source list.
Select `SENSOR_PORT_MODE_CUSTOM_ENCODER` and verify startup, CRC errors, direction, and sampling behavior before motor operation.
The adapter and motor-control integration still require bench validation.

## Validation

From the support directory, run:

```sh
cmake -S . -B build/host
cmake --build build/host
ctest --test-dir build/host --output-on-failure
python3 -m unittest discover -s tests -p "test_*.py"
python3 tools/validate_pinmap.py --allow-tbd
```

The host tests check gate-disable ordering, pin ownership, ADC ordering, nominal scaling, and encoder bus pins.
Python tests verify required-signal checks, duplicate detection, release blockers, and the firmware build guard.
Strict pin validation must fail while the recorded blockers remain open.
A passing draft check confirms worksheet structure only.

The board has not been flashed, powered, or commissioned by this implementation.
