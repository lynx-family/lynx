# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from typing import Callable, List, Optional

from core.base.result import Ok, Result
from core.target.target import Target


class Builder:
    def __init__(self):
        pass

    def pre_action(self, skip: Optional[Callable[[], bool]] = None) -> Result:
        return Ok()

    def build(self, target: Target) -> Result:
        raise NotImplementedError("Builder subclasses must implement build()")

    def build_all(self, targets: List[Target]) -> Result:
        """Build targets sequentially, stopping after the first failure.

        Subclasses may override this method to build multiple targets together.
        """
        for target in targets:
            result = self.build(target)
            if result.is_err():
                return result
        return Ok()
