# -*- coding: UTF-8 -*-
# Copyright 2021 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import time
from lynx_e2e.api.lynx_view import LynxView

from .testcase import Testcase


class TestcaseCheckResult(Testcase):
    def __init__(self, name, url, attributes):
        super().__init__(name, url, attributes)
        self.result_list = []

    def get_result_list(self, node):
        value = node.attributes.get('lynx-test-tag')
        if value and value.startswith('result'):
            self.result_list.append(node)

        for child in node.children:
            self.get_result_list(child)

    def check_result(self):
        if len(self.result_list) == 0:
            raise RuntimeError("The node that needs to be checked does not exist in the current page.")

        for i, result in enumerate(self.result_list):
            text_node = result.children[0]
            text = text_node.attributes.get('text')
            if text != "success":
                raise RuntimeError(f"The inspection of the result failed. The content is{text}")

    def run(self, test, _):
        if 'sleep' in self.attr:
            time.sleep(self.attr['sleep'])
        else:
            time.sleep(1)
        lynxview = test.app.get_lynxview('lynxview', LynxView)
        tree = lynxview.local_document_tree()
        self.get_result_list(tree.root)
        self.check_result()
        self.result_list.clear()
