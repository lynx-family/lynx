# -*- coding: UTF-8 -*-
# Copyright 2021 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import errno
import time
from retrying import retry
import socket
import traceback
from lynx_e2e.api.logger import EnumLogLevel
from lynx_e2e.api.exception import LynxCDPTimeoutException

from .case_set import CaseSet
from .mixin.result_mixin import TestCaseStatus

PLATFORM = os.environ.get('platform')
RETRY_MAX_TIME = 2


def create_instance(module_name, class_name, *args, **kwargs):
    module_meta = __import__(module_name, globals(), locals(), [class_name])
    class_meta = getattr(module_meta, class_name)
    obj = class_meta(*args, **kwargs)
    return obj


class TestRunner:
    def __init__(self, test):
        self._cases = []
        self._plugins = []
        self._test = test
        self._platform = PLATFORM
        self._category = ''
        self.enable_scale = True
        self._open_card = True
        self._case_run = True
        self._runner_result_map = {}

    def get_cases(self):
        return tuple(self._cases)

    def use_scale(self, enable):
        """decide open card with specify width and height"""
        self.enable_scale = enable

    def set_open_card(self, enable):
        """decide whether the framework is needed to open the test page"""
        self._open_card = enable

    def set_case_run(self, enable):
        """decide whether the case's script is executed"""
        self._case_run = enable

    def add_plugin(self, plugin):
        self._plugins.append(plugin)

    def clear_plugin(self):
        self._plugins = []

    def add_case(self, case):
        if type(case) is CaseSet:
            self._category = case._category
            self._cases.extend(case.get_cases())
        else:
            self._cases.append(case)

    def clear_case(self):
        self._cases = []

    def pre_test(self):
        current_path = os.path.dirname(os.path.abspath(__file__))
        for _ in range(5):
            current_path = os.path.dirname(current_path)
        if PLATFORM == 'android' and self.enable_scale:
            self._test.log_info("set density")
            self._test.device.shell_command('wm density 320')
            self._test.log_info(self._test.device.shell_command('wm density'))
            time.sleep(1)
        elif PLATFORM == 'ios':
            pass

    def post_test(self):
        self._test.app.disconnect()
        if PLATFORM == 'android':
            self._test.device.shell_command('wm density reset')
        elif PLATFORM == 'ios':
            pass

    def run_test(self):
        if 'platform' not in os.environ:
            raise RuntimeError('Environment variable \"platform\" not set')

        time.sleep(2)
        self.pre_test()
        for p in self._plugins:
            p.pre_test()

        self.run_test_recursor()
        self._test.set_current_case(None)
        self._test.start_step('---------------  Exec finished, print result --------------- ')
        self.print_task_result()
        test_result = self._test.get_test_result()
        test_status = test_result['test_status']
        if test_status == TestCaseStatus.SUCCESS:
            self._test.log_info("Caseset passed!")
        else:
            for name, testcase_result in test_result['test_result'].items():
                if testcase_result.status == TestCaseStatus.FAIL:
                    exception = testcase_result.exec_list[-1].get('exception', None)
                    traceback = testcase_result.exec_list[-1].get('traceback', None)
                    self._test.print_log_list_by_case(name, exception, traceback)

        for p in self._plugins:
            self._test.log_info(f"{p.__class__.__name__} post_test")
            p.post_test()

        self.post_test()

    def _need_retry_function(test) -> bool:
        return len(test.get_retry_list()) != 0

    @retry(stop_max_attempt_number=RETRY_MAX_TIME, retry_on_result=_need_retry_function)
    def run_test_recursor(self):
        self._test.set_current_case(None)

        test_result = self._test.get_test_result()
        is_first_test = (test_result['max_retry_time'] == 0)
        case_list = self._cases if is_first_test else self._test.get_retry_list()
        if len(case_list) == 0:
            return self._test
        self._test.start_step(f'--------{"Execute" if is_first_test else "Retry"} test, there are %s test cases-------' % len(case_list))
        for case in self._cases:
            if case.name not in case_list and case not in case_list:
                continue
            self._test.add_test_result(case.name, case.attr['fail_retry_time'])
            if case.attr['restart_before_exec']:
                self.restart_and_connect_app()

            try:
                self._test.set_current_case(case)
                # Clear the log list corresponding to the case,
                # and only keep the log of the last execution of the case in the logCache.
                self._test.log_cache.clear_logs_by_key(case.name)

                time.sleep(5)
                self._test.start_step(
                    '--------%s case [ %s ]-------' % ("Exec", case.name))
                def _run_case():
                    for p in self._plugins:
                        p.pre_run(case)
                    # Open the corresponding page of the test case
                    if self._open_card:
                        self._test.log_info('open card: ' + case.url)
                        self._test.app.open_card(case.url)

                    # There is a time cost from processing the cdp message for opening the page to updating the session_id. 
                    # To avoid the disorder of the session_id, add a waiting time before case run.
                    time.sleep(1)
                    if self._case_run:
                        case.run(self._test, self._platform)

                    for p in self._plugins:
                        p.post_run(case)
                try:
                    _run_case()
                except LynxCDPTimeoutException as e:
                    self._test.log_info('-------- Meets LynxCDPTimeoutException, restart app --------')
                    self.restart_and_connect_app()
                    # After capturing the exception, restart the app and rerun the current test case once.
                    _run_case()
                except socket.error as e:
                    if e.errno == errno.EPIPE:
                        self._test.log_info('-------- Meets Socket Broken Pipe Exception, restart app --------')
                        self.restart_and_connect_app()
                        # After capturing the exception, restart the app and rerun the current test case once.
                        _run_case()
                    else:
                        raise e
                self._test.update_test_result(case.name, TestCaseStatus.SUCCESS)
            except Exception as e:
                self._test.update_test_result(case.name, TestCaseStatus.FAIL, {
                    'status': TestCaseStatus.FAIL,
                    'exception': e,
                    'traceback': traceback.format_exc(),
                })
                self._test.log_record('Case [%s] failed: %s\n%s' % (case.name, str(e), traceback.format_exc()), level=EnumLogLevel.WARNING)
                if self._test.app.is_app_crashed() == False:
                    self._test.log_record("Case %s meets app crash, raise error!" % case.name, EnumLogLevel.WARNING)
                    break
        return self._test

    def restart_and_connect_app(self):
        self._test.log_info("restart app!")
        self._test.app.restart()
        time.sleep(3)
        self._test.app.connect_app_to_lynx_server()

    def print_task_result(self):
        test_result = self._test.get_test_result()
        max_retry_time = test_result['max_retry_time']
        result_detail = test_result['test_result']
        for i in range(max_retry_time):
            if i == 0:
                task_name = 'First round'
            else:
                task_name = 'Retry round'
            round_result_status = TestCaseStatus.SUCCESS
            output = "\n{:^20}".format('%s exec result' % task_name)
            output += "\n{:<25} {:<30}".format('fail_case', 'exception')
            output += "\n{:<25} {:<30}".format(
                    '------------', '-------------')
            for name, case_result in result_detail.items():
                result_list = case_result.exec_list
                if result_list is not None and len(result_list) > i:
                    if result_list[i]['status'] == TestCaseStatus.FAIL:
                        round_result_status = TestCaseStatus.FAIL
                        exception = str(result_list[i]['exception'])
                        output += "\n{:<25} {:<25}".format(name, exception)
            if round_result_status == TestCaseStatus.SUCCESS:
                self._test.log_info('%s exec pass!' % task_name)
            else:
                self._test.log_info(output)
