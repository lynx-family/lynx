# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from lynx_e2e.api.logger import EnumLogLevel, LogRecord


class LogObject:

    '''
        enum log type
    '''
    TEXT = 0
    RECODE_OBJ = 1

    def __init__(self, log_type, msg, level=None,
                   stack=None, attachments={},
                   exception=None, override_level=False, has_error=False):
        self.type = log_type
        self.msg = msg
        self.level = level
        self.stack = stack
        self.attachments = attachments
        self.exception = exception
        self.override_level = override_level
        self.has_error = has_error

class LogCache:
    def __init__(self):
        self._cache_map = {}

    def add_log(self, key, data_obj):
        if key not in self._cache_map:
            self._cache_map[key] = []
            self._cache_map[key].append(data_obj)
        elif isinstance(self._cache_map[key], list) :
            self._cache_map[key].append(data_obj)

    def get_logs_by_key(self, key):
        if key not in self._cache_map:
            return []
        else:
            return self._cache_map[key]

    def clear_logs_by_key(self, key):
        if key in self._cache_map:
            self._cache_map[key] = []

class LogRecordMixin:
    def __init__(self):
        self.current_case = None

    def set_current_case(self, case):
        self.current_case = case

    def log_info(self, msg, level=EnumLogLevel.INFO):
        log_obj = LogObject(LogObject.TEXT, msg, level)
        if hasattr(self, 'current_case') and self.current_case != None:
            self.log_cache.add_log(self.current_case.name, log_obj)
        super().log_info(msg)

    def log_record(self, msg, level=EnumLogLevel.INFO,
                   stack=None, attachments={},
                   exception=None, override_level=False, has_error=False, hide_in_report=False):
        case_name = attachments.get('case_name', None)
        if case_name is not None:
            del attachments['case_name']
            if hide_in_report == False:
                log_obj = LogObject(LogObject.RECODE_OBJ, msg, level, stack, attachments, exception, override_level, has_error)
                self.log_cache.add_log(case_name, log_obj)
        record = LogRecord(stack=stack,
                           attachments=attachments,
                           exception=exception)
        self.test_result.log_record(level, msg, record=record, override_level=override_level)

    def print_log_list_by_case(self, case_name, exception, traceback):
        self.start_step('--------------- Logs for failed use case [ %s ]--------------' % case_name)
        log_list = self.log_cache.get_logs_by_key(case_name)
        for log_obj in log_list:
            if log_obj.type == LogObject.TEXT:
                if log_obj.level == EnumLogLevel.ERROR:
                    super().log_record(log_obj.msg, EnumLogLevel.ERROR)
                else:
                    super().log_info(log_obj.msg)
            elif log_obj.type == LogObject.RECODE_OBJ:
                super().log_record(log_obj.msg, [log_obj.level, EnumLogLevel.WARNING][log_obj.has_error==True], log_obj.stack, log_obj.attachments, log_obj.exception, log_obj.override_level)
        self.test_result.log_record( EnumLogLevel.ERROR, 'Error: %s\n%s\n--------------- Use case [ %s ] failed, please check the log above --------------' % (str(exception), [traceback, ''][traceback == None], case_name), record=LogRecord(stack=None, attachments={}, exception=None))