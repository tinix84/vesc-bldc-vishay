#!/usr/bin/env python3
"""Validate the schematic pin worksheet without third-party dependencies."""

from __future__ import annotations

import argparse
import csv
import re
import sys
import tomllib
from collections import defaultdict
from pathlib import Path


CORE_SIGNALS = {
    "pwm_u_high",
    "pwm_u_low",
    "pwm_v_high",
    "pwm_v_low",
    "pwm_w_high",
    "pwm_w_low",
    "gate_enable",
    "gate_fault",
    "current_u",
    "current_v",
    "current_w",
    "phase_u_voltage",
    "phase_v_voltage",
    "phase_w_voltage",
    "dc_bus_voltage",
    "mosfet_temperature",
    "can_rx",
    "can_tx",
    "usb_dm",
    "usb_dp",
    "swdio",
    "swclk",
}

PROTOCOL_SIGNALS = {
    "bissc": {
        "raik060_biss_clock",
        "raik060_biss_data",
        "raik060_biss_mosi_reserve",
        "raik060_biss_nss_reserve",
    },
    "spi": {
        "raik060_spi_clock",
        "raik060_spi_data_in",
        "raik060_spi_chip_select",
    },
}

HALL_SIGNALS = {"hall_1", "hall_2", "hall_3"}
PIN_PATTERN = re.compile(r"^P[A-K](?:1[0-5]|[0-9])$")


def load_inputs(project_path: Path, pinmap_path: Path) -> tuple[dict, list[dict[str, str]]]:
    with project_path.open("rb") as project_file:
        project = tomllib.load(project_file)
    with pinmap_path.open(newline="", encoding="utf-8-sig") as pinmap_file:
        rows = list(csv.DictReader(pinmap_file))
    return project, rows


def validate(project: dict, rows: list[dict[str, str]], allow_tbd: bool) -> list[str]:
    errors: list[str] = []
    by_signal = {}
    for row in rows:
        signal = row.get("signal", "").strip()
        if not signal:
            errors.append("signal name is blank")
        elif signal in by_signal:
            errors.append(f"{signal}: duplicate signal row")
        by_signal[signal] = row
    required = set(CORE_SIGNALS)
    profile = project.get("control", {}).get("profile", "standard")
    if profile == "encoder_only_foc":
        required.difference_update({"phase_u_voltage", "phase_v_voltage", "phase_w_voltage"})
    elif profile != "standard":
        errors.append(f"unsupported control profile: {profile}")
    protocol = str(project.get("raik060", {}).get("protocol", "TBD")).lower()

    if protocol in PROTOCOL_SIGNALS:
        required.update(PROTOCOL_SIGNALS[protocol])
    elif not allow_tbd:
        errors.append("board/project.toml: raik060.protocol must be 'bissc' or 'spi'")

    if project.get("raik060", {}).get("coexist_with_motor_halls", False):
        required.update(HALL_SIGNALS)

    missing_rows = sorted(required - set(by_signal))
    if missing_rows:
        errors.append("missing signal rows: " + ", ".join(missing_rows))

    assigned: defaultdict[str, list[str]] = defaultdict(list)
    for signal, row in by_signal.items():
        pin = row.get("pin", "").strip().upper()
        if signal in required and pin in {"", "TBD", "NC"} and not allow_tbd:
            errors.append(f"{signal}: required pin is still {pin or 'blank'}")
            continue
        if pin in {"", "TBD", "NC"}:
            continue
        if not PIN_PATTERN.fullmatch(pin):
            errors.append(f"{signal}: unsupported MCU pin syntax '{pin}' (expected e.g. PA8)")
        assigned[pin].append(signal)
        if signal in required and row.get("peripheral", "").strip().upper() in {"", "TBD", "NC"}:
            errors.append(f"{signal}: peripheral/resource is unresolved")

    for pin, signals in sorted(assigned.items()):
        if len(signals) > 1:
            errors.append(f"{pin}: assigned more than once ({', '.join(signals)})")

    for field in ("mcu", "mcu_package", "power_stage_voltage_max", "phase_current_max"):
        if str(project.get(field, "TBD")).upper() == "TBD" and not allow_tbd:
            errors.append(f"board/project.toml: {field} is still TBD")

    if not allow_tbd:
        for field in ("phase_voltage_sensing_available", "vdda_feed_verified",
                      "schematic_pcb_parity_verified", "shutdown_behavior_verified",
                      "power_stage_limits_verified"):
            if field == "phase_voltage_sensing_available" and profile == "encoder_only_foc":
                continue
            if project.get("hardware_review", {}).get(field) is False:
                errors.append(f"board/project.toml: hardware_review.{field} is not verified")
        sensor = project.get("raik060", {})
        if "ordering_code" in sensor and str(sensor["ordering_code"]).upper() in {"", "TBD"}:
            errors.append("board/project.toml: raik060.ordering_code is still TBD")
        if sensor.get("frame_kind_verified") is False:
            errors.append("board/project.toml: RAIK060 frame kind is not verified")
        analog = project.get("analog", {})
        if analog.get("bus_transfer_calibrated") is False:
            errors.append("board/project.toml: bus-voltage transfer is not calibrated")
        if "pcb_ntc_beta" in analog and str(analog["pcb_ntc_beta"]).upper() in {"", "TBD"}:
            errors.append("board/project.toml: PCB NTC curve is still TBD")

    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--project", type=Path, default=Path("board/project.toml"))
    parser.add_argument("--pinmap", type=Path, default=Path("board/pinmap.csv"))
    parser.add_argument(
        "--allow-tbd",
        action="store_true",
        help="validate worksheet structure while allowing unresolved assignments",
    )
    args = parser.parse_args()

    project, rows = load_inputs(args.project, args.pinmap)
    errors = validate(project, rows, args.allow_tbd)
    if errors:
        for error in errors:
            print(f"ERROR: {error}", file=sys.stderr)
        return 1

    assigned = sum(row.get("pin", "").strip().upper() not in {"", "TBD", "NC"} for row in rows)
    print(f"pin map structure valid: {len(rows)} signals, {assigned} assigned")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

