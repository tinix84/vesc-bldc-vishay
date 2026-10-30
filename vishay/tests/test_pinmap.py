from __future__ import annotations

import importlib.util
import shutil
import subprocess
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("validate_pinmap", ROOT / "tools" / "validate_pinmap.py")
assert SPEC and SPEC.loader
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class PinMapValidationTests(unittest.TestCase):
    def test_draft_workspace_is_structurally_valid(self) -> None:
        project, rows = MODULE.load_inputs(ROOT / "board" / "project.toml", ROOT / "board" / "pinmap.csv")
        self.assertEqual(MODULE.validate(project, rows, allow_tbd=True), [])

    def test_duplicate_pin_is_rejected(self) -> None:
        project = {"mcu": "STM32", "mcu_package": "LQFP", "power_stage_voltage_max": "60", "phase_current_max": "80", "raik060": {"protocol": "spi", "coexist_with_motor_halls": False}}
        rows = [
            {"signal": signal, "pin": f"PA{index}", "peripheral": "GPIO"}
            for index, signal in enumerate(sorted(MODULE.CORE_SIGNALS | MODULE.PROTOCOL_SIGNALS["spi"]))
        ]
        rows[1]["pin"] = rows[0]["pin"]
        errors = MODULE.validate(project, rows, allow_tbd=False)
        self.assertTrue(any("assigned more than once" in error for error in errors))

    def test_required_nc_is_rejected(self) -> None:
        project, rows = MODULE.load_inputs(ROOT / "board" / "project.toml", ROOT / "board" / "pinmap.csv")
        rows = [dict(row, pin="NC") if row["signal"] == "gate_enable" else row for row in rows]
        errors = MODULE.validate(project, rows, allow_tbd=False)
        self.assertTrue(any("gate_enable: required pin is still NC" in error for error in errors))

    def test_encoder_only_profile_does_not_require_phase_adc(self) -> None:
        project, rows = MODULE.load_inputs(ROOT / "board" / "project.toml", ROOT / "board" / "pinmap.csv")
        errors = MODULE.validate(project, rows, allow_tbd=False)
        self.assertFalse(any(error.startswith("phase_") or "phase_voltage_sensing_available" in error for error in errors))
        self.assertTrue(any("vdda_feed_verified" in error for error in errors))

    def test_standard_profile_still_requires_phase_adc(self) -> None:
        project, rows = MODULE.load_inputs(ROOT / "board" / "project.toml", ROOT / "board" / "pinmap.csv")
        project["control"]["profile"] = "standard"
        errors = MODULE.validate(project, rows, allow_tbd=False)
        self.assertTrue(any(error.startswith("phase_u_voltage:") for error in errors))

    def test_duplicate_signal_is_rejected_even_during_draft(self) -> None:
        project, rows = MODULE.load_inputs(ROOT / "board" / "project.toml", ROOT / "board" / "pinmap.csv")
        errors = MODULE.validate(project, rows + [dict(rows[0])], allow_tbd=True)
        self.assertTrue(any("duplicate signal row" in error for error in errors))

    def test_assigned_required_pin_needs_known_resource(self) -> None:
        project, rows = MODULE.load_inputs(ROOT / "board" / "project.toml", ROOT / "board" / "pinmap.csv")
        rows = [dict(row, pin="PA8", peripheral="TBD") if row["signal"] == "pwm_u_high" else row for row in rows]
        errors = MODULE.validate(project, rows, allow_tbd=True)
        self.assertTrue(any("pwm_u_high: peripheral/resource is unresolved" in error for error in errors))

    def test_review_blockers_are_not_hidden_by_complete_pin_rows(self) -> None:
        project, rows = MODULE.load_inputs(ROOT / "board" / "project.toml", ROOT / "board" / "pinmap.csv")
        errors = MODULE.validate(project, rows, allow_tbd=False)
        self.assertTrue(any("hardware_review.vdda_feed_verified is not verified" in error for error in errors))
        self.assertTrue(any("RAIK060 frame kind is not verified" in error for error in errors))
        self.assertTrue(any("PCB NTC curve is still TBD" in error for error in errors))

    @unittest.skipUnless(shutil.which("cc"), "C preprocessor unavailable")
    def test_firmware_header_refuses_unreviewed_target(self) -> None:
        result = subprocess.run(
            ["cc", "-E", "-x", "c", str(ROOT / "port/vesc/hwconf/hw_vishay_vesc.h")],
            capture_output=True, text=True, check=False,
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Vishay port blocked: VDDA", result.stderr)


if __name__ == "__main__":
    unittest.main()

