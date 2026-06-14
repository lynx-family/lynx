# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# /usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import tempfile

from env_setup import API_DOC_ANNOTATION


def build_api_metadata_content(object_list):
    api_body = "".join(api_object.get_api_str() for api_object in object_list)
    if not api_body.strip():
        return None
    return API_DOC_ANNOTATION + api_body


def write_api_metadata(api_file, object_list, platform):
    content = build_api_metadata_content(object_list)
    if content is None:
        print(
            f"Refuse to update {platform} API metadata because generated metadata is empty. "
            f"Kept existing file: {api_file}",
            file=sys.stderr,
        )
        return False

    api_file = os.path.abspath(api_file)
    api_dir = os.path.dirname(api_file)
    temp_path = None
    try:
        os.makedirs(api_dir, exist_ok=True)
        with tempfile.NamedTemporaryFile(
            "w",
            encoding="utf-8",
            dir=api_dir,
            prefix=f".{os.path.basename(api_file)}.",
            suffix=".tmp",
            delete=False,
        ) as temp_file:
            temp_path = temp_file.name
            temp_file.write(content)
        os.replace(temp_path, api_file)
        temp_path = None
        return True
    except OSError as e:
        print(
            f"Failed to update {platform} API metadata: {e}. "
            f"Kept existing file: {api_file}",
            file=sys.stderr,
        )
        return False
    finally:
        if temp_path and os.path.exists(temp_path):
            try:
                os.remove(temp_path)
            except OSError as e:
                print(
                    f"Failed to remove temporary API metadata file {temp_path}: {e}",
                    file=sys.stderr,
                )
