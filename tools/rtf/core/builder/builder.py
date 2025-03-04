# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from typing import Callable

from core.target.target import Target


class Builder:
    def __init__(self):
        pass

    def pre_action(self, skip: Callable[[], bool] = None):
        pass

    def build(self, target: Target):
        pass
