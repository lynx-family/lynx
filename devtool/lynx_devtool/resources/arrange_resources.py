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
    logbox_zip = os.path.join(input, "logbox.zip") if input else None
    error_parser = os.path.join(input, "lynx-error-parser.js") if input else None
    switch_page_source = (
        os.path.join(input, "switchPage", "devtoolSwitch.lynx.bundle")
        if input
        else None
    )

    if not (input and os.path.exists(logbox_zip) and os.path.exists(error_parser)):
        print(f"The devtool resource directory {input} is invalid.")
        return
    if output:
        os.makedirs(output, exist_ok=True)
    else:
        return

    # unzip
    logbox_output = os.path.join(output, "logbox")
    if os.path.exists(logbox_output):
        shutil.rmtree(logbox_output)
    shutil.unpack_archive(logbox_zip, logbox_output)

    # copy
    shutil.copy2(error_parser, logbox_output)

    if switch_page_source and os.path.exists(switch_page_source):
        switch_page_output_dir = os.path.join(output, "switchPage")
        switch_page_output = os.path.join(
            switch_page_output_dir, "devtoolSwitch.lynx.bundle")

        if os.path.abspath(switch_page_source) != os.path.abspath(switch_page_output):
            if os.path.exists(switch_page_output_dir):
                shutil.rmtree(switch_page_output_dir)
            os.makedirs(switch_page_output_dir, exist_ok=True)
            shutil.copy2(switch_page_source, switch_page_output)

if __name__ == "__main__":
    build()
