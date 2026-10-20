"""Recalculate the two defense tables without treating them as new measurements."""

import csv
from decimal import Decimal
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def summarize(filename, from_voltage):
    with (ROOT / "data" / filename).open(encoding="utf-8", newline="") as source:
        rows = list(csv.DictReader(source))

    print(f"\n{filename}")
    print("set_mA,calculated_or_reported_mA,deviation_mA,deviation_percent")
    relative_errors = []
    for row in rows:
        target = Decimal(row["set_current_mA"])
        if from_voltage:
            current = (
                Decimal(row["reported_voltage_V"])
                / Decimal(row["nominal_load_ohm"])
                * 1000
            )
        else:
            current = Decimal(row["reported_current_mA"])
        deviation = current - target
        if target:
            relative = deviation / target * 100
            relative_errors.append(abs(relative))
            relative_text = f"{relative:+.6f}"
        else:
            relative_text = "N/A (zero setpoint)"
        print(f"{target},{current:.6f},{deviation:+.6f},{relative_text}")

    print(f"maximum absolute relative deviation: {max(relative_errors):.6f}%")


if __name__ == "__main__":
    print("Historical table recalculation only; excludes measurement uncertainty.")
    summarize("defense-slide08-current-calibration.csv", from_voltage=True)
    summarize("defense-slide16-current-calibration.csv", from_voltage=False)
