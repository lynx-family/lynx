# -*- coding: UTF-8 -*-
# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.


class Testcase:
    def __init__(self, name, url, attributes):
        self.name = name
        self.url = url
        self.image_prefix = ""
        self.attr = attributes

    def run(self, test, platform):
        raise NotImplementedError
