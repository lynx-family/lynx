# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import subprocess
from time import sleep

from core.utils.log import Log


class EmulatorEnv:
    def __init__(self):
        self.env = os.environ
        android_home = self.env["ANDROID_HOME"]
        system_path = self.env["PATH"]
        system_path += f":{android_home}/platform-tools"
        system_path += f":{android_home}/emulator"
        system_path += f":{android_home}/cmdline-tools/latest/bin"
        system_path += f":{android_home}/tools/bin"
        self.env["PATH"] = system_path
        self.device_process = None
        self.device_backend_log = "device_backend.log"
        self.device = None
        self.is_root_rule = False

    def close(self):
        if self.device_process is not None:
            self.device_process.kill()

    def prepare_android_emulator(self, use_real_device=False, use_root=True):
        if use_real_device:
            devices = self.get_real_devices()
            if len(devices) == 0:
                Log.fatal("No real device resources found!")
            self.device = devices[0]
            if use_root:
                self.start_real_device_use_root()
        else:
            devices = self.get_simulator_devices()
            if len(devices) == 0:
                devices = self.try_start_emulator()
                if len(devices) == 0:
                    self.close()
                    Log.fatal("No simulator device resources found!")
            self.device = devices[0]
            if use_root:
                self.start_simulator_use_root()

    def start_real_device_use_root(self):
        # TODO(zhaosong.lmm):
        # On a physical device, you can use 'adb pull'
        # to fetch files and generate coverage without needing root permissions.
        self.is_root_rule = True
        return
        # check_root_cmd = f"adb -s {self.device} shell which su"
        # try:
        #     output = subprocess.check_output(check_root_cmd, shell=True, env=self.env)
        #     check_result = output.decode("utf-8").strip()
        #     if "/su" in check_result:
        #         self.is_root_rule = True
        #     else:
        #         Log.warning(
        #             f"Please root the phone({self.device}) manually!"
        #             f"An unrooted phone won't affect the testing, but it won't generate coverage."
        #         )
        # except Exception as e:
        #     Log.warning(
        #         f"Please root the phone({self.device}) manually!"
        #         f"An unrooted phone won't affect the testing, but it won't generate coverage."
        #     )

    def start_simulator_use_root(self):
        root_cmd = f"adb -s {self.device} root"
        Log.info(f"Restart device {self.device} with root")
        subprocess.check_call(root_cmd, shell=True)
        for i in range(20):
            Log.info(f"Waiting for simulator {self.device} restart!")
            devices = self.get_devices_list(False)
            if self.device in devices:
                # TODO(zhaosong.lmm)
                # The reason for waiting for 10 seconds is that the device will restart after running "adb root".
                # I tried to determine the successful restart by checking if the target device appears in "adb devices",
                # but there were still issues. Therefore, I added a delay to wait for the restart.
                sleep(10)
                Log.success(f"Restart Simulator with root role {self.device} success!")
                self.is_root_rule = True
                return
            else:
                sleep(5)
        Log.warning(f"Restart Simulator with root role {self.device} failed!")

    def get_real_devices(self):
        return self.get_devices_list(True)

    def get_simulator_devices(self):
        return self.get_devices_list(False)

    def get_devices_list(self, real_device):
        cmd = "adb devices"
        output = subprocess.check_output(cmd, shell=True, env=self.env)
        devices = [
            device.split("\t")
            for device in output.decode("utf-8").strip().split("\n")
            if device.endswith("device")
        ]
        result = []
        for device in devices:
            if device[0].startswith("emulator") and not real_device:
                result.append(device[0])
            else:
                if real_device:
                    result.append(device[0])
        return result

    def try_start_emulator(self):
        devices = []
        output = subprocess.check_output(
            "emulator -list-avds", shell=True, env=self.env
        )
        images = output.decode("utf-8").strip().split("\n")
        if len(images) == 0:
            Log.fatal("No simulator resources found!")
        start_simulator_device_cmd = (
            f"emulator -no-window -writable-system -avd {images[0]}"
        )
        log_file = open(self.device_backend_log, "w")
        self.device_process = subprocess.Popen(
            [start_simulator_device_cmd], shell=True, stdout=log_file
        )

        for i in range(20):
            Log.info(f"Waiting for simulator {images[0]} start!")
            devices = self.get_devices_list(False)
            if len(devices) == 0:
                sleep(5)
            else:
                return devices
        return []
