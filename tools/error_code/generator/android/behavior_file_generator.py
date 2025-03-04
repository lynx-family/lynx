# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from common import *
from generator.base_generator import *
from generator.android.android_base import *

__all__ = ["ErrorBehaviorFileGenerator"]

BEHAVIOR_NAME_PREFIX = "EB_"

class ErrorBehaviorFileGenerator(FileGenerator):
    def __init__(self, relative_path, file_name):
        super().__init__(relative_path, file_name)
        # registration must be done in the order of product codes
        self._register_child_generator(ErrorBehaviorDefGenerator("\t"))

    def _generate_file_header(self):
        self._append(PACKAGE_DECL)
        self._append(
            "public class {0} {{\n".format(ERR_BEHAVIOR_CLASS_NAME))

    def _generate_file_footer(self):
        self._append("}\n\n")

class ErrorBehaviorDefGenerator(BehaviorGenerator):
    def before_gen_behavior(self, behavior, section):
        super().before_gen_behavior(behavior, section)
        # generate behavior code
        behavior_name = BEHAVIOR_NAME_PREFIX
        raw_name = section[KEY_NAME] + behavior[KEY_NAME]
        raw_name = raw_name.replace(VALUE_DEFAULT, "")
        behavior_name += pascal_to_upper_snake(raw_name)
        behavior_code = section[KEY_HIGH_CODE] * 100 + behavior[KEY_MID_CODE]
        self._append_with_indent(
            "public static final int {0} = {1};\n".format(
                behavior_name, behavior_code))

    def after_gen_behavior(self):
        self._append("\n")