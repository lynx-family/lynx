# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# /usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import subprocess
import sys
from jinja2 import Environment, FileSystemLoader
from env_setup import API_CONFIG, DOXYGEN_PATH, LYNX_ROOT_PATH
from api_utils import remove_dirs


class DoxygenConfig:
    def __init__(
        self,
        platform,
        enable_generate_xml=False,
        enable_generate_html=False,
    ):
        self.enable_generate_xml = enable_generate_xml
        self.enable_generate_html = enable_generate_html
        self.input = " ".join(
            f"{os.path.join(LYNX_ROOT_PATH, path)}"
            for path in API_CONFIG[platform][
                "metadata_path" if enable_generate_xml else "doc_path"
            ]
        )

        if platform == "android":
            self.doxygen_config_path = os.path.join(
                os.path.dirname(__file__),
                os.pardir,
                os.pardir,
                os.pardir,
                os.pardir,
                "platform",
                "android",
                "api",
                "doxygen.cfg",
            )
        elif platform == "ios":
            self.doxygen_config_path = os.path.join(
                os.path.dirname(__file__),
                os.pardir,
                os.pardir,
                os.pardir,
                os.pardir,
                "platform",
                "darwin",
                "ios",
                "api",
                "doxygen.cfg",
            )

    def prepare_config(self):
        if not self.doxygen_config_path:
            print("doxygen config path not set")
            return None
        env = Environment(
            loader=FileSystemLoader(os.path.dirname(self.doxygen_config_path))
        )
        template = env.get_template(os.path.basename(self.doxygen_config_path))
        config = template.render(
            config=self,
        )

        new_config_path = self.doxygen_config_path + ".new"
        with open(new_config_path, "w") as f:
            f.write(config)

        return new_config_path

    def execute(self, api_path, platform) -> bool:
        target_config_path = self.prepare_config()
        if not target_config_path:
            print("prepare config failed")
            return False
        command = [DOXYGEN_PATH, target_config_path]
        if self.enable_generate_xml:
            remove_dirs(os.path.join(api_path, platform, "xml"))
        if self.enable_generate_html:
            remove_dirs(os.path.join(api_path, platform, "html"))

        try:
            result = subprocess.run(
                command,
                capture_output=True,
                text=True,
                cwd=api_path,
                check=True,
            )
        except subprocess.CalledProcessError as e:
            print(f"generate api metadata failed: {e.stderr}", file=sys.stderr)
            return False
        finally:
            if platform == "ios" or platform == "android":
                os.remove(target_config_path)

        return True
