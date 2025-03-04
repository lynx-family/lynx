# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
from common import *
from generator.base_generator import *
from generator.darwin.darwin_base import *

__all__ = ["BehaviorSrcGenerator"]

class BehaviorSrcGenerator(FileGenerator):
    def __init__(self, relative_path, file_name):
        super().__init__(relative_path, file_name)
        self._register_child_generator(BehaviorCodeDefGenerator())

    def _generate_file_header(self):
        self._append(
            "#import \"{0}{1}\"\n\n".format(
                ERR_BEHAVIOR_CLASS_NAME, HEADER_FILE_EXT))


class BehaviorCodeDefGenerator(DarwinBehaviorGenerator):
    def before_gen_behavior(self, behavior, section):
        super().before_gen_behavior(behavior, section)
        name = get_behavior_code_name(behavior, section)
        code = self._get_behavior_code(behavior, section)
        self._append_with_indent("NSInteger const ")
        self._append("{0} = {1};\n".format(name, code))

    def after_gen_behavior(self):
        self._append("\n")


    