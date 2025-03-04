# -*- coding: UTF-8 -*-
# Copyright 2021 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from __future__ import unicode_literals
import difflib
import json
import os
import time
from lynx_e2e.api.lynx_view import LynxView
from lynx_e2e.api.config import settings

from .testcase import Testcase
from lynx_e2e.api.logger import EnumLogLevel


class RedBoxError(Exception):
    """Red Box error
    """
    pass


class TestcaseTestbench(Testcase):
    """
    check testbench
    """

    def run(self, test, platform):
        case_id = self.attr['id']
        check_image = self.attr['check_image']
        check_red_box = self.attr['check_red_box'] if 'check_red_box' in self.attr else True
        app = test.app
        if check_red_box:
            test.start_step('--------start check red box %s;-------' % self.attr['name'])
            if app.get_red_box().wait_for_existing(timeout=5, interval=0.5, raise_error=False):
                raise RedBoxError("red box existing!!")
        test.start_step('--------wait replay file %s;-------' % self.attr['name'])
        stream_id = app.end_replay()
        file_path = os.path.join('%s_%d.txt' % ('testbench', int(time.time())))
        replay_file = app.io_read_testbench(stream_id, output_file=file_path)
        test.start_step('--------start screenshot %s;-------' % self.attr['name'])
        lynx_view = test.app.get_lynxview('lynxview', LynxView)        
        if check_image:
            image_name = '%s.png' % case_id
            test.start_step('--------diff with baseline image in resource/testbench--------')
            test.diff_img(app=test.app, name=image_name)
        test.start_step('--------start check replay file %s;-------' % self.attr['name'])
        replay_diff_file = 'testbench%s_diff.txt' % case_id
        replay_baseline_file = os.path.join(settings.PROJECT_ROOT, 'resources', platform, 'testbench',
                                            '%s.txt' % case_id)
        if not os.path.exists(replay_baseline_file):
            test.log_record("Baseline file is not exist", EnumLogLevel.INFO, attachments={case_id + "test_file": replay_file}, has_error=True)
            raise Exception("replay_baseline_file %s not find!!" % replay_baseline_file)
        if not self.check_file(replay_diff_file, replay_baseline_file, replay_file):
            test.log_record("File diff failed", EnumLogLevel.INFO, attachments={case_id + "test_file": replay_file, case_id + "baseline_file": replay_baseline_file,
                             case_id + "file_diff": replay_diff_file}, has_error=True)
            raise Exception("check replay file failed %s %s" % (case_id, self.attr['name']))

    def check_file(self, diff_file, baseline_file, test_file):
        text1 = open(baseline_file).readlines()
        text2 = open(test_file).readlines()
        replay_baseLine_temp_file = os.path.join(settings.PROJECT_ROOT, 'testbench_temp.txt' )
        baseline_text = ""
        for text in text1:
            baseline_text += text
        text_json = json.loads(baseline_text)
        with open(replay_baseLine_temp_file, "w") as f:
            json.dump(text_json, f, indent=2, ensure_ascii=False)

        text1 = open(replay_baseLine_temp_file).readlines()

        diff_lines = []
        for line in difflib.unified_diff(text1, text2):
            diff_lines.append(line)
        if len(diff_lines) > 0:
            with open(diff_file, 'w') as f:
                f.writelines(diff_lines)
            return False
        return True
