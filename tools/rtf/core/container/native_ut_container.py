# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import time
from datetime import datetime

from core.builder.builder_manager import BuilderManager
from core.container.container import Container
from core.coverage.coverage import Coverage
from core.coverage.coverage_factory import CoverageFactory
from core.target.observer import LogObserver, CrashObserverFactory, OwnersObserver
from core.target.target import Target
from core.target.target_factory import TargetFactory
from core.utils.log import Log


class NativeUTContainer(Container):
    def __init__(self, builder, coverage, test_type):
        super().__init__()
        self.parallel_queue = []
        self.serial_queue = []
        self.builder_manager: BuilderManager = BuilderManager(builder)
        self.coverage: Coverage = CoverageFactory(coverage)
        self.test_type = test_type
        self.observers = [LogObserver()]
        crash_observer = CrashObserverFactory()
        if crash_observer is not None:
            self.observers.append(crash_observer)
        self.observers.append(OwnersObserver())

    def before_test(self, targets, filter: str):
        self.builder_manager.pre_action()
        for t in targets.keys():
            if filter != "all" and t != filter:
                continue
            target = TargetFactory(self.test_type, targets[t], t)
            if not target.enable:
                continue
            enable_parallel = (
                targets[t]["enable_parallel"]
                if "enable_parallel" in targets[t]
                else False
            )
            self.add_targets(target, enable_parallel)
        for target in self.serial_queue:
            self.builder_manager.build(target)
        for target in self.parallel_queue:
            self.builder_manager.build(target)

    def after_test(self):
        self.coverage.gen_report(self.serial_queue + self.parallel_queue)

    def kill_all_process(self):
        for target in self.parallel_queue:
            target.kill()

    def __print_running_process(self, names: [str]):
        if len(names) <= 3:
            for name in names:
                Log.warning(f"{name} is running!")

    def wait_for_execution_finish(self, timeout=20 * 60):
        end_time = time.time() + timeout
        over_processes_list = []
        only_run_process = [target.name for target in self.parallel_queue]
        while time.time() < end_time:
            all_processes_stop = True
            for target in self.parallel_queue:
                if target.is_end():
                    if target.has_error():
                        if target.retry != target.retry_count:
                            target.retry_count = target.retry_count + 1
                            Log.error(
                                f"{target.name} has error! retry({target.retry_count}/{target.retry})"
                            )
                            target.run()
                            all_processes_stop = False
                        else:
                            Log.error(f"{target.name} has error!")
                            for observer in self.observers:
                                observer.update(target)
                            self.kill_all_process()
                            return target.process.returncode
                    elif target.name not in over_processes_list:
                        over_processes_list.append(target.name)
                        only_run_process.remove(target.name)
                        current_time = datetime.timestamp(datetime.now())
                        external_message = ""
                        if target.retry_count != 0:
                            external_message = (
                                f"retry pass ({target.retry_count}/{target.retry})"
                            )
                        Log.success(
                            f"[{len(over_processes_list)}/{len(self.parallel_queue)}] {target.name} run success! "
                            f"CostTime ({int((current_time - target.start_time)*1000)}ms) {external_message}"
                        )
                        self.__print_running_process(only_run_process)
                else:
                    all_processes_stop = False
            if all_processes_stop:
                return 0
            time.sleep(2)
        for target in self.parallel_queue:
            if not target.is_end():
                Log.error(f"{target.name} timeout!")
        self.kill_all_process()
        return -1

    def test(self):
        for target in self.serial_queue:
            target.run().wait()
            if target.has_error():
                Log.error(f"{target.name} has error!")
                for observer in self.observers:
                    observer.update(target)
                Log.fatal(f"Test Failed with ret-code {target.process.returncode}")
            else:
                Log.success(f"{target.name} run success!")

        for target in self.parallel_queue:
            target.run_pre_actions()
            target.run()

        ret_code = self.wait_for_execution_finish()
        if ret_code != 0:
            Log.fatal(f"Test Failed with ret-code {ret_code}")

    def add_targets(self, target: Target, parallel=False):
        if parallel:
            self.parallel_queue.append(target)
        else:
            self.serial_queue.append(target)
