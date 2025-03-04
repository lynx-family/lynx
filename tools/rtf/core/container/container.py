# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.


class Container:
    def __init__(self):
        pass

    def run(self, targets, filter: str):
        self.before_test(targets, filter)
        self.test()
        self.after_test()

    def before_test(self, targets, filter: str):
        pass

    def after_test(self):
        pass

    def test(self):
        pass
