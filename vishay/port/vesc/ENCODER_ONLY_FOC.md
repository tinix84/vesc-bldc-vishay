# Encoder-only FOC development profile

The `HW_ENCODER_ONLY_FOC` profile implements field-oriented control without phase-voltage sensors.
It requires three current sensors, bus-voltage measurement, and a valid absolute custom encoder.
The implementation targets the pinned VESC baseline `4b4dd3062dcbb3dbdd188b8dd2d3e6b88ad69da8`.
The public fork contains the core changes and the support directory.

## Hardware hold

The schematic remains unchanged.
The native files do not establish a VDDA supply feed.
Verify VDDA supply and continuity on the actual board before firmware operation.
Firmware cannot supply VDDA.

The Vishay hardware header retains its unconditional compile error.
`ENABLE_GATE()` still clears PB5.
GPIO initialization keeps all six PWM pads as inputs.
There is no approval flag or VDDA bypass.
No Vishay image has been flashed or commissioned.
VDDA verification alone does not approve gate activation.

## Control behavior

| Function | Profile behavior |
|---|---|
| Rotor angle and speed | Absolute custom encoder at all speeds, including coasting |
| Observer fallback | Disabled |
| Driven voltage model | Dead-time-corrected modulation multiplied by `2 * Vbus / 3` |
| Coasting voltage model | Inactive internal state, without measured back-EMF |
| Phase-voltage telemetry and offsets | Unavailable (`NAN`), never invented ADC values |
| Current-offset calibration | Stationary motor, gates disabled, 1000 samples, no phase ADC reads |
| Accepted control modes | Current, current brake, duty, speed, and position |
| Unsupported modes | BLDC, DC, sensorless, HFI, open-loop, handbrake, and phase override |
| Unsupported features | Field weakening, current decoupling, automatic motor detection, and encoder alignment |
| Unsupported diagnostics | Raw phase ADC, debug sample streaming, and virtual motor connection |

The driven voltage model estimates applied voltage.
It does not measure phase voltage or determine coasting back-EMF.
The profile bypasses observer updates and selects encoder angle for the speed trackers.
Standard targets retain their original code paths.

## Configuration and encoder readiness

The profile requires `MOTOR_TYPE_FOC`, `FOC_SENSOR_MODE_ENCODER`, and `SENSOR_PORT_MODE_CUSTOM_ENCODER`.
It also requires corrected encoder speed, startup current calibration, disabled phase filters, zero field weakening, and disabled current decoupling.
Encoder ratio, encoder offset, motor resistance, and motor inductance must contain finite values.
Ratio, resistance, and inductance must be positive.
These checks do not establish physical calibration or approved limits.

Unsupported stored configuration keeps command input inhibited.
The boot path forces restricted mode fields before selecting a motor implementation.
Unsupported live configuration is rejected.
The ADC interrupt also checks configuration, encoder health, calibration completion, and control mode.

Confirm the purchased RAIK060 frame before calling `hw_vishay_start_encoder(frame_kind)`.
The adapter does not guess a frame or start automatically.
Start the adapter before motor initialization for startup calibration.
It waits 160 ms before sampling and requests a 100 us interval between transfers.
Transfer time and RTOS scheduling add to that interval.
The GPIO SPI bus uses a 1 us half-period.

The first valid sample establishes readiness.
An invalid frame, failed thread, missing sample, CRC error, sensor error, or stale sample prevents operation.
The restricted profile faults after one erroneous transfer.
The default freshness limit is 1000 us.
This development limit requires review against measured latency and maximum electrical speed.
No observer supplies replacement angle data.

Startup calibration waits up to five seconds for a valid encoder.
It rejects encoder movement above 0.5 degrees, driver faults, invalid current offsets, or a running motor.
A timeout or failed calibration leaves calibration incomplete.
The implementation does not accept default offsets after a timeout.
Encoder alignment and motor parameters require independently verified values because automatic detection is disabled.

## Validation

Four host C tests cover protocol decoding, GPIO inhibition, configuration restrictions, voltage modeling, and encoder health.
Nine Python tests check worksheet validation and the hardware guard.
The encoder-health tests include stale samples, sensor errors, CRC errors, and RTOS tick wrap.

The ARM compile test uses `tests/arm_smoke/hw_encoder_only_smoke.h` on a maintained reference target.
It disables gate enable and replaces every phase ADC identifier with an invalid identifier.
An unguarded phase ADC dependency therefore fails compilation.
The reference build is a compile test only.
Never flash its output or use it as the Vishay hardware configuration.
CI does not publish its binaries.
The standard VESC6 MK3 build checks the original paths with this profile disabled.

Local validation used Arm GNU Toolchain `14.3.rel1`.
Both ARM builds and all host tests passed before publication.
Compilation does not validate motor control, timing, analog measurements, or protection behavior on hardware.

## Remaining board requirements

Verify actual VDDA, sensor frame, current polarity, current scaling, bus transfer, and ADC reference.
Confirm the NTC curve, physical SWD access, shutdown behavior, and peripheral ownership.
Approve voltage, current, temperature, regeneration, and gate-driver protection limits.
Review the DRV8301 initialization sequence before enabling its register writes.
Keep gates disabled until these requirements and a separate commissioning review are complete.

## Private framework and public integration

The private framework keeps its upstream submodule unchanged.
Its `patches/0001-encoder-only-foc.patch` records the core integration changes.
For review, use a separate upstream clone and place the framework support files under `vishay/`.
Apply the patch only in that separate clone.
The public fork already contains the integration changes and needs no patch application.
