# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import subprocess

from core.builder.builder_manager import BuilderManager
from core.container.container import Container
from core.coverage.coverage import Coverage
from core.coverage.coverage_factory import CoverageFactory
from core.target.observer import LogObserver, OwnersObserver
from core.target.target_factory import TargetFactory
from core.utils.emu_env_setup import EmulatorEnv
from core.utils.log import Log


class AndroidUTContainer(Container):
    def __init__(self, builder, coverage):
        super().__init__()
        self.targets = []
        self.test_type = "android-ut"
        self.builder_manager: BuilderManager = BuilderManager(builder)
        self.coverage: Coverage = CoverageFactory(coverage)
        self.emulator = EmulatorEnv()
        self.observers = [LogObserver(), OwnersObserver()]
        self.device_log = "device.log"
        self.log_process = None
        self.use_real_device = False
        self.clean = True

    def before_test(self, targets, filter: str):
        self.emulator.prepare_android_emulator(use_real_device=self.use_real_device)

        def skip():
            return not self.clean

        self.builder_manager.pre_action(skip=skip)
        for t in targets.keys():
            if filter != "all" and t != filter:
                continue
            target = TargetFactory(self.test_type, targets[t], t)
            if not target.enable:
                continue
            self.targets.append(target)

        for target in self.targets:
            self.builder_manager.build(target)
            target.insert_global_info("device_name", self.emulator.device)

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

    def test(self):
        log_file = open(self.device_log, "w")
        self.log_process = subprocess.Popen(
            ["adb", "logcat", "-v", "time"], stdout=log_file, stderr=subprocess.STDOUT
        )
        for target in self.targets:
            target.run()
            if target.has_error():
                for observer in self.observers:
                    observer.update(target)
                Log.fatal(f"{target.name} has error!")
            else:
                Log.success(f"{target.name} success!")
