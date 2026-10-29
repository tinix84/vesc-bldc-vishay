# Vishay firmware support

This directory contains experimental support for a Vishay motor controller and the RAIK060 position sensor.
The initial support files come from firmware framework commit `0e718ff`.
The firmware baseline is upstream VESC commit `4b4dd3062dcbb3dbdd188b8dd2d3e6b88ad69da8`.

## Status

The portable RAIK060 SPI protocol driver has host tests.
The VESC custom-encoder adapter requires hardware integration and bench tests.
The board worksheet contains unresolved values.
The hardware header intentionally stops compilation until the pin map and electrical values are reviewed.
Do not flash this placeholder target or energize an unverified power stage.

This directory contains firmware support and draft configuration templates.
It does not contain board design exports or internal project reviews.

## Host validation

Run these commands from the repository root.
Use Python 3.11 or later, CMake 3.20 or later, and a C11 compiler.

```sh
cmake -S vishay -B build/vishay-host
cmake --build build/vishay-host
ctest --test-dir build/vishay-host --output-on-failure
python3 -m unittest discover -s vishay/tests -p "test_*.py"
python3 vishay/tools/validate_pinmap.py --project vishay/board/project.toml --pinmap vishay/board/pinmap.csv --allow-tbd
```

The final command checks the draft worksheet structure.
It does not confirm that the board is ready for use.
Before a hardware build, complete `vishay/board/project.toml` and `vishay/board/pinmap.csv`.
Then run the validator without `--allow-tbd`.

## Hardware integration

Read [the integration guide](port/vesc/INTEGRATION.md).
Keep the complete SPI peripheral available for the native BiSS-C driver until hardware tests establish the required connections.
The RAIK060 SPI variant uses the separate protocol driver and custom-encoder adapter.
It is not compatible with the AS5047 SPI protocol.

Complete the hardware target only after the schematic, pin assignments, and analog scaling values are reviewed.
Verify gate control, current limits, ADC scaling, and sensor behavior with a current-limited supply before connecting a motor.
This repository does not include a validated Vishay firmware binary.

## License

The project-authored files use `GPL-3.0-or-later` where their SPDX headers specify that license.
The [GPL version 3 text](LICENSE) accompanies this support directory.
Upstream files retain their existing licenses and copyright notices.
