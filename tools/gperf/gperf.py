#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

"""Shared helpers for running the build host's gperf executable."""

import argparse
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
        "to the host gperf binary in args.gn."
    )


def run_gperf(gperf, template, arguments):
    gperf = str(Path(gperf).resolve())
    template = Path(template).resolve()
    generated = subprocess.check_output(
        [gperf, *arguments, template.name],
        cwd=template.parent,
        text=True,
        encoding="utf-8",
    )
    generated = re.sub(r"\bregister\s+", "", generated)
    if "__fallthrough__" not in generated and "[[fallthrough]]" not in generated:
        generated = generated.replace("/*FALLTHROUGH*/", "[[fallthrough]];")
    return generated


def write_if_changed(output, contents):
    output = Path(output)
    if output.exists() and output.read_text(encoding="utf-8") == contents:
        return
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", encoding="utf-8", newline="\n") as destination:
        destination.write(contents)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--find-gperf", metavar="BUILDTOOLS_DIR")
    args = parser.parse_args()
    if not args.find_gperf:
        parser.error("--find-gperf is required")
    try:
        print(find_gperf(args.find_gperf))
    except OSError as error:
        parser.exit(1, f"gperf discovery failed: {error}\n")


if __name__ == "__main__":
    main()
