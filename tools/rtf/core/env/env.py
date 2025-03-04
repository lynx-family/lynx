# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import platform
from core.utils.log import Log


class RTFConfig:
    def __init__(self):
        # default plugins
        self.plugins = ["Init"]

    def build(self, config_path):
        with open(config_path, "r") as config_file:
            local_env = {}
            exec(config_file.read(), local_env)
            if "plugins" in local_env:
                self.plugins += local_env["plugins"]


class RTFEnv:
    env_map = {}
    config: RTFConfig = RTFConfig()

    @staticmethod
    def set_env(key, value):
        RTFEnv.env_map[key] = value

    @staticmethod
    def get_project_root_path():
        return (
            RTFEnv.env_map["project_root_path"]
            if "project_root_path" in RTFEnv.env_map
            else None
        )

    @staticmethod
    def get_rtf_root_path():
        return (
            RTFEnv.env_map["rtf_root_path"]
            if "rtf_root_path" in RTFEnv.env_map
            else None
        )

    @staticmethod
    def get_env(key):
        if key not in RTFEnv.env_map:
            return None
        return RTFEnv.env_map[key]

    @staticmethod
    def init_project_env():
        current_dir = os.path.abspath("")
        while True:
            rtf_file = os.path.join(current_dir, ".rtf")
            if os.path.exists(rtf_file):
                RTFEnv.env_map["project_root_path"] = os.path.dirname(rtf_file)
                RTFEnv.env_map["rtf_root_path"] = os.path.abspath(rtf_file)
                RTFEnv.config.build(os.path.join(RTFEnv.get_rtf_root_path(), "config"))
                break
            if current_dir == os.path.dirname(current_dir):
                Log.warning(
                    f"The current directory has not been initialized, you can run `rtf init project` to init."
                )
                return False
            current_dir = os.path.dirname(current_dir)
        system = platform.system().lower()
        if system == "darwin":
            RTFEnv.env_map["os"] = "darwin"
        elif system.startswith("linux"):
            RTFEnv.env_map["os"] = "linux"
        else:
            RTFEnv.env_map["os"] = "undefined"

        RTFEnv.env_map["DEBUG"] = True if "RTFDEBUG" in os.environ else False

        return True
