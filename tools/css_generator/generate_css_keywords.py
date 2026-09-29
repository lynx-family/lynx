#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

"""Generate the CSS keyword lookup table with the host's gperf tool."""

import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys


def find_gperf(buildtools_dir):
    if sys.platform == "darwin":
        return "/usr/bin/gperf"
    executable = "gperf.exe" if sys.platform == "win32" else "gperf"
    packaged = Path(buildtools_dir) / "gperf" / "bin" / executable
    if packaged.is_file():
        return str(packaged.resolve())
    installed = shutil.which(executable)
    if installed:
        return installed
    raise FileNotFoundError(
        "gperf was not found. Sync the build tools or set gperf_executable "
        "to the host gperf binary in args.gn.")


def generate(gperf, template, output):
    template = Path(template).resolve()
    generated = subprocess.check_output(
        [gperf, "-D", "-t", template.name], cwd=template.parent,
        text=True, encoding="utf-8")
    # gperf 3.0.x (macOS and Windows) emits the removed C++17 keyword.
    generated = re.sub(r"\bregister\s+", "", generated)
    # Newer gperf releases already emit fallthrough attributes.
    if "__fallthrough__" not in generated and "[[fallthrough]]" not in generated:
        generated = generated.replace("/*FALLTHROUGH*/", "[[fallthrough]];")
    generated = re.sub(r"^/\* Command-line: .*\*/$",
                       "/* Generated from css_keywords.tmpl. Do not edit. */",
                       generated, flags=re.MULTILINE)
    output = Path(output)
    if output.exists() and output.read_text(encoding="utf-8") == generated:
        return
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", encoding="utf-8", newline="\n") as destination:
        destination.write(generated)


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
