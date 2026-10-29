#!/usr/bin/env python3
# Copyright vedri studio Ltd. All Rights Reserved.
#
# Every validation code the Core layer can emit must be listed in docs/VALIDATION_CODES.md, which is
# where the English and Welsh messages are reviewed. Fails (exit 1) on codes missing from the doc and
# on doc rows for codes that no longer exist.
#
# Usage: check_validation_codes.py <repo root>

import pathlib
import re
import sys


def main() -> int:
    root = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
    core = root / "VedriVPToolkit/Source/VedriVPToolkitEditor/Public/Core"
    doc = root / "docs/VALIDATION_CODES.md"

    emitted = set()
    for header in sorted(core.glob("*.h")):
        text = header.read_text(encoding="utf-8")
        # The code is the first string literal inside each AddIssue(...) call.
        for match in re.finditer(r"AddIssue\(([^;]*?)\"([a-z][A-Za-z]*\.[A-Za-z0-9]+)\"", text, re.S):
            emitted.add(match.group(2))

    documented = set(re.findall(r"^\|\s*`([a-z][A-Za-z]*\.[A-Za-z0-9]+)`", doc.read_text(encoding="utf-8"), re.M))

    missing = sorted(emitted - documented)
    stale = sorted(documented - emitted)
    for code in missing:
        print(f"missing from docs/VALIDATION_CODES.md: {code}")
    for code in stale:
        print(f"documented but never emitted: {code}")
    if missing or stale:
        return 1
    print(f"validation codes: {len(emitted)} emitted, all documented")
    return 0


if __name__ == "__main__":
    sys.exit(main())
