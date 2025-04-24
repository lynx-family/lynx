# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.


class Result:
    def __init__(self, code, msg):
        self.code = code
        self.msg = msg

    def is_ok(self):
        pass

    def is_err(self):
        return not self.is_ok()


class Err(Result):
    def __init__(self, code, msg):
        super().__init__(code, msg)

    def is_ok(self):
        return False


class Ok(Result):
    def __init__(self):
        super().__init__(None, None)

    def is_ok(self):
        return True
