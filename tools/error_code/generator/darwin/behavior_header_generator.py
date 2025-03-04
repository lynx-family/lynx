# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
from common import *
from generator.base_generator import *
from generator.darwin.darwin_base import *

__all__ = ["BehaviorHeaderGenerator"]

class BehaviorHeaderGenerator(FileGenerator):
    def __init__(self, relative_path, file_name):
        super().__init__(relative_path + "Public/", file_name)
        self._register_child_generator(BehaviorCodeDeclGenerator())

    def _generate_file_header(self):
        self._append("#import <Foundation/Foundation.h>\n\n")
        self._append("NS_ASSUME_NONNULL_BEGIN\n\n")

    def _generate_file_footer(self):
        self._append("NS_ASSUME_NONNULL_END\n")

class BehaviorCodeDeclGenerator(DarwinBehaviorGenerator):
    def before_gen_behavior(self, behavior, section):
        super().before_gen_behavior(behavior, section)
        name = get_behavior_code_name(behavior, section)
        self._append_with_indent("FOUNDATION_EXPORT NSInteger const ")
        self._append("{0};\n".format(name))

    def after_gen_behavior(self):
        self._append("\n")


    