#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import sys
import shutil

# arrange the devtool resources as platform required
def build():
    input = sys.argv[1] if len(sys.argv) > 1 else None
    output = sys.argv[2] if len(sys.argv) > 2 else None

    if not (input and os.path.exists(f"{input}/logbox.zip") and os.path.exists(f"{input}/lynx-error-parser.js")):
        print(f"The devtool resource directory {input} is invalid.")
        return
    if output:
        os.makedirs(output, exist_ok=True)
    else:
        return

    # unzip
    if os.path.exists(f"{output}/logbox"):
        shutil.rmtree(f"{output}/logbox")
    shutil.unpack_archive(f"{input}/logbox.zip", f"{output}/logbox")

    # copy
    shutil.copy2(f"{input}/lynx-error-parser.js", f"{output}/logbox")

if __name__ == "__main__":
    build()
