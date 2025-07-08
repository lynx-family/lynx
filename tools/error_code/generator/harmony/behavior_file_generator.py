# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from common import *
from generator.base_generator import *
from generator.harmony.harmony_common import *

__all__ = ["ErrBehaviorFileGenerator"]

class ErrBehaviorFileGenerator(FileGenerator):
    def __init__(self, relative_path, file_name):
        super().__init__(relative_path, file_name)
        self._format_off = "\n"
        self._format_on = "\n"
        self._child_generators.append(
            ErrorBehaviorDefGenerator("\t"))

    def _generate_file_header(self):
        self._append("\n")
        self._append(
            "export class {0} {{\n".format(ERR_BEHAVIOR_CLASS_NAME))

    def _generate_file_footer(self):
        self._append("}\n")

class ErrorBehaviorDefGenerator(BehaviorGenerator):
    def before_gen_behavior(self, behavior, section):
        super().before_gen_behavior(behavior, section)
        # generate behavior code
        behavior_name = get_behavior_code_name(behavior, section)
        behavior_code = self._get_behavior_code(behavior, section)
        self._append_with_indent(
            "public static readonly {0}: number = {1};\n".format(
                behavior_name, behavior_code))

    def after_gen_behavior(self):
        self._append("\n")