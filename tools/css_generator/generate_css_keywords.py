#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

"""Generate the CSS keyword lookup table with the host's gperf tool."""

import argparse
import os
from pathlib import Path
import re
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from gperf.gperf import find_gperf, run_gperf, write_if_changed


def generate(gperf, template, output):
    generated = run_gperf(gperf, template, ["-D", "-t"])
    generated = re.sub(r"^/\* Command-line: .*\*/$",
                       "/* Generated from css_keywords.tmpl. Do not edit. */",
                       generated, flags=re.MULTILINE)
    write_if_changed(output, generated)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--find-gperf", metavar="BUILDTOOLS_DIR")
    parser.add_argument("--gperf", help="Path to the host gperf executable")
    parser.add_argument("--input", help="Path to css_keywords.tmpl")
    parser.add_argument("--output", help="Path to the generated C++ source")
    args = parser.parse_args()
    try:
        if args.find_gperf:
            print(find_gperf(args.find_gperf))
            return
        if not args.input or not args.output:
            parser.error("--input and --output are required")
        gperf = args.gperf or find_gperf(
            Path(__file__).resolve().parents[2] / "buildtools")
        generate(os.path.abspath(gperf), args.input, args.output)
    except (OSError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"CSS keyword generation failed: {error}\n")


if __name__ == "__main__":
    main()
