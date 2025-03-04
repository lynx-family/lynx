# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from common import *
from checker.base import *
from checker.metadata_checker import *
from checker.error_code_def_checker import *

__all__ = ["SpecChecker"]

class SpecChecker:
    def __init__(self, spec):
        self._spec = spec

    def check(self):
        if self._spec == None:
            return False
        metadata_checker = MetadataChecker(self._spec.get(KEY_METADATA, None))
        if not metadata_checker.check():
            return False
        
        err_code_checker = ErrorCodeDefChecker(self._spec)
        err_code_checker.prepare_check(
            metadata_checker.declared_metadata_keywords(), 
            metadata_checker.keyword_to_metadata())
        if not err_code_checker.check():
            return False
        return True
