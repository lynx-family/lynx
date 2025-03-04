# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os.path
import subprocess

from core.env.env import RTFEnv
from core.target.target import Target
from core.utils.log import Log


class Observer:
    def __init__(self, name):
        self.name = name

    def update(self, target: Target):
        Log.info(f"🔔👇🏼👇🏼👇🏼👇🏼[Message From {self.name} for {target.name}]")
        self.action(target)
        Log.info(f"🔔👆🏼👆🏼👆🏼👆🏼[Message From {self.name} for {target.name}]")

    def action(self, target: Target):
        pass


class LogObserver(Observer):
    def __init__(self):
        super().__init__("LogObserver")

    def action(self, target: Target):
        target.print_log()


class OwnersObserver(Observer):
    def __init__(self):
        super().__init__("OwnersObserver")

    def action(self, target: Target):
        Log.print(
            "You can contact the owner for assistance in troubleshooting the issue!"
        )
        index = 1
        for owner in target.owners:
            Log.print(f"[{index}]: {owner}")
            index += 1


class CrashObserver(Observer):
    def __init__(self):
        super().__init__("CrashObserver")

    def action(self, target: Target):
        pass


class DarwinCrashObserver(CrashObserver):
    def __init__(self):
        super().__init__()

    def action(self, target: Target):
        if not target.has_crash():
            Log.warning(f"{target.name} did not crash, exit.")
            return
        lldb_file_name = os.path.join(RTFEnv.get_project_root_path(), "command.lldb")
        lldb_command_file = open(lldb_file_name, "w")
        lldb_command_file.write(f"file {target.target_path}\n")
        lldb_command_file.write(
            f'process launch -- {" ".join(target.params["args"] if "args" in target.params else [])}'
        )
        lldb_command_file.close()
        lldb_analysis_cmd = f"lldb -s {lldb_file_name} -o bt -o q"
        lldb_output = subprocess.check_output(
            lldb_analysis_cmd, cwd=target.params["cwd"], shell=True
        )
        Log.print(lldb_output.decode("utf-8"))


class LinuxCrashObserver(CrashObserver):
    def __init__(self):
        super().__init__()
        subprocess.check_call("ulimit -c unlimited", shell=True)
        subprocess.check_call(
            'sysctl -w "kernel.core_pattern=core.%p" >/dev/null', shell=True
        )

    def action(self, target: Target):
        if not target.has_crash():
            return
        core_file = os.path.join(target.params["cwd"], f"core.{target.process.pid}")
        if os.path.exists(core_file):
            lldb_core_analysis_cmd = f'lldb -c {core_file} -o "bt" -o "q"'
            lldb_output = subprocess.check_output(lldb_core_analysis_cmd, shell=True)
            Log.print(lldb_output.decode("utf-8"))


def CrashObserverFactory():
    if RTFEnv.env_map["os"] == "darwin":
        return DarwinCrashObserver()
    elif RTFEnv.env_map["os"] == "linux":
        return LinuxCrashObserver()
    else:
        return None
