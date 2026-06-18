# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import subprocess

from core.builder.builder_manager import BuilderManager
from core.container.container import Container
from core.coverage.coverage import Coverage
from core.coverage.coverage_factory import CoverageFactory
from core.target.observer import LogObserver, OwnersObserver, AndroidCrashObserver
from core.target.target_factory import TargetFactory
from core.utils.emu_env_setup import EmulatorEnv
from core.utils.log import Log
from core.base.result import Ok, Err
from core.base.constants import Constants


class AndroidUTContainer(Container):
    def __init__(self, builder, coverage):
        super().__init__()
        self.targets = []
        self.test_type = "android-ut"
        self.builder_manager: BuilderManager = BuilderManager(builder)
        self.coverage_init_params = coverage
        self.coverage: Coverage = None
        self.emulator = EmulatorEnv()
        self.observers = [LogObserver(), OwnersObserver(), AndroidCrashObserver()]
        self.device_log = "device.log"
        self.log_process = None
        self.use_real_device = False
        self.clean = True

    def restart_device_handler(self):
        self.emulator.close()
        self.emulator.prepare_android_emulator(use_real_device=self.use_real_device)
        for target in self.targets:
            target.insert_global_info("device_name", self.emulator.device)

    def before_test(self, targets, filter: str):
        result = self.emulator.prepare_android_emulator(
            use_real_device=self.use_real_device
        )

        if result.is_err():
            return result

        coverage = CoverageFactory(self.coverage_init_params)
        if coverage.is_err():
            return coverage

        self.coverage = coverage.get_value()

        def skip():
            return not self.clean

        result = self.builder_manager.pre_action(skip=skip)
        if result.is_err():
            return result
        for t in targets.keys():
            if filter != "all" and t != filter:
                continue
            result = TargetFactory(self.test_type, targets[t], t)
            if result.is_err():
                return result
            target = result.get_value()
            if not target.enable:
                continue
            self.targets.append(target)

        # Guard against a silent no-op: when --target names a target that does
        # not exist or is disabled, self.targets stays empty and test() /
        # after_test() would return Ok(), making app.py print "success!" with
        # nothing built or run.
        if not self.targets:
            if filter == "all":
                return Err(
                    Constants.TARGET_BUILD_ERR,
                    "No enabled targets to run. Check that at least one target "
                    "has 'enable: true' in the template.",
                )
            available = ", ".join(
                t for t in targets.keys() if targets[t].get("enable", True)
            )
            return Err(
                Constants.TARGET_BUILD_ERR,
                f"Target '{filter}' not found or disabled. "
                f"Available enabled targets: {available}",
            )
        for target in self.targets:
            result = self.builder_manager.build(target)
            if result.is_err():
                return result
            target.insert_global_info("device_name", self.emulator.device)
            target.insert_global_info(
                "restart_device_handler", self.restart_device_handler
            )

        return Ok()

    def after_test(self):
        try:
            if self.emulator.is_root_rule:
                self.coverage.gen_report(self.targets)
            else:
                Log.warning(
                    f"Device has no root permission, skipping coverage generation"
                )
        except Exception as e:
            pass
        finally:
            self.emulator.close()
            self.log_process.kill()
            return Ok()

    def init_device_logcat(self):
        log_file = open(self.device_log, "w")
        subprocess.check_call(f"adb -s {self.emulator.device} logcat -c", shell=True)
        self.log_process = subprocess.Popen(
            ["adb", "-s", f"{self.emulator.device}", "logcat", "-v", "time"],
            stdout=log_file,
            stderr=subprocess.STDOUT,
        )

    def test(self):
        self.init_device_logcat()
        for target in self.targets:
            result = target.run()
            if result.is_err():
                return result
            target.wait()
            if target.has_error():
                for observer in self.observers:
                    observer.update(target)
                return Err(Constants.TARGET_RUN_ERR, f"{target.name} has error!")
            else:
                Log.success(f"{target.name} success!")
        return Ok()
