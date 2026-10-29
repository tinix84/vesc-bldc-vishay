# VESC integration seam

This fork starts from upstream commit `4b4dd3062dcbb3dbdd188b8dd2d3e6b88ad69da8`.
Keep board-specific support under `vishay/`. The intended upstream-compatible seam is a
new `hwconf/vishay` target plus, only if the SPI RAIK060 is selected, the small
driver in this repository.

## Preferred path: RAIK060 BiSS-C

Current VESC firmware already implements `SENSOR_PORT_MODE_BISSC`. Select an
18-bit, single-turn RAIK060 BiSS-C part and provide the RS-422 line driver and
receiver required by the sensor. Populate VESC's `HW_SPI_*` macros and set
`m_encoder_counts` to 18 in the motor configuration. The existing VESC driver
uses SCK and MISO for the physical link, but its configuration also reserves
MOSI and NSS pins; keep a complete SPI peripheral available until bench tests
prove a smaller mapping.

The existing driver does not expose the RAIK060 multi-turn counter. Treat
multi-turn BiSS-C as new firmware work, not as a drop-in configuration.

## Fallback path: RAIK060 SPI

`vishay/src/raik060.c` implements the simple read-only 28-bit and 44-bit frames from
the Vishay datasheet, including CRC-8 and status handling. The VESC adapter in
`vishay/port/vesc/raik060_vesc.c` samples it in a thread and registers VESC's existing
custom-encoder callbacks.

To integrate after the hardware target exists:

1. Add `vishay/src/raik060.c` and `vishay/port/vesc/raik060_vesc.c` to `ENCSRC` in a small
   upstream patch, and add the `vishay/include` directory to `INC`.
2. In `hw_init_gpio()`, configure dedicated CS, SCLK and MISO pins, create a
   `raik060_vesc_config_t`, and call `raik060_vesc_start()` once.
3. Set the motor sensor port mode to `SENSOR_PORT_MODE_CUSTOM_ENCODER` and the
   FOC sensor mode to encoder in VESC Tool.
4. Keep MOSI routed if register access, offset programming, air-gap check or
   self-calibration commands are wanted. It is not needed for simple position
   frames.
5. Verify CRC, direction, electrical zero, sampling jitter and fault behavior
   on a current-limited bench before enabling the power stage.

The portable driver uses one-microsecond half periods by default in the adapter
example (about 500 kHz after software overhead), within the RAIK060's specified
100 kHz to 3 MHz range. The first-clock delay is at least 5 us and the default
sample period is 100 us.

## Local VESC build command

Once `hw_vishay_vesc.c/.h` are complete, build without modifying upstream's
board list:

```sh
make fw_custom \
  HW_SRC="$PWD/vishay/port/vesc/hwconf/hw_vishay_vesc.c" \
  HW_HEADER="$PWD/vishay/port/vesc/hwconf/hw_vishay_vesc.h"
```

After successful bring-up, move the target under upstream
`hwconf/vishay/`. That makes the hardware discoverable by the VESC build and is
the natural shape for a pull request and eventual firmware packaging.

