# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from enum import Enum


class TestCaseStatus(Enum):
    SUCCESS = 'success'
    FAIL = 'fail'
    RUNNING = 'running'
    UNKNOWN = 'unknown'


class TestCaseResult:
    def __init__(self, name, max_retry_time, status = TestCaseStatus.UNKNOWN):
        self.name = name
        self.max_retry_time = max_retry_time
        self.exec_list = []
        self.status = status

    def update_exec_result(self, status: TestCaseStatus=None, exec_info: dict=None):
        if status:
            if status == TestCaseStatus.FAIL and len(self.exec_list) >= self.max_retry_time:
                self.status = status
            elif status == TestCaseStatus.SUCCESS:
                self.status = status
            else:
                self.status = TestCaseStatus.RUNNING
        if exec_info:
            self.exec_list.append(exec_info)

class ResultMixin:
    def __init__(self):
        self._test_result = {}

    def add_test_result(self, name, max_retry_time=1):
        if name in self._test_result:
            return
        testcase_result = TestCaseResult(name, max_retry_time)
        self._test_result[name] = testcase_result

    def update_test_result(self, name, status=None, exec_info: dict=None):
        if name not in self._test_result:
            raise RuntimeError("do not find testcase %s" % name)
        testcase_result = self._test_result[name]
        testcase_result.update_exec_result(status, exec_info)

    def get_retry_list(self):
        result = []
        for name, testcase_result in self._test_result.items():
            if testcase_result.status in [TestCaseStatus.RUNNING, TestCaseStatus.UNKNOWN]:
                result.append(name)
        return result

    def get_test_result(self):
        max_retry_time = 0
        test_status = TestCaseStatus.SUCCESS
        for _, testcase_result in self._test_result.items():
            max_retry_time = max(max_retry_time, len(testcase_result.exec_list))
            if testcase_result.status == TestCaseStatus.FAIL:
                test_status = TestCaseStatus.FAIL
        return {
            "test_status": test_status,
            "max_retry_time": max_retry_time,
            "test_result": self._test_result
        }
