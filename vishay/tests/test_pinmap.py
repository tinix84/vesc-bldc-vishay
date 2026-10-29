from __future__ import annotations

import importlib.util
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


if __name__ == "__main__":
    unittest.main()

