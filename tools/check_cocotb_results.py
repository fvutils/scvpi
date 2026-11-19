#!/usr/bin/env python3

from __future__ import annotations

import sys
import xml.etree.ElementTree as ET
from pathlib import Path


def main() -> int:
    results_path = Path(sys.argv[1] if len(sys.argv) > 1 else "results.xml")

    if not results_path.is_file():
        print(f"ERROR: cocotb results file not found: {results_path}")
        return 1

    try:
        root = ET.parse(results_path).getroot()
    except ET.ParseError as exc:
        print(f"ERROR: invalid cocotb results file: {exc}")
        return 1

    testcases = list(root.iter("testcase"))
    failures = list(root.iter("failure"))
    errors = list(root.iter("error"))
    skipped = list(root.iter("skipped"))

    if not testcases:
        print("ERROR: cocotb results contain no test cases")
        return 1

    if failures or errors or skipped:
        print(
            f"cocotb regression failed: "
            f"{len(testcases)} test(s), "
            f"{len(failures)} failure(s), "
            f"{len(errors)} error(s), "
            f"{len(skipped)} skipped test(s)"
        )
        return 1

    print(f"cocotb regression passed: {len(testcases)} test(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())