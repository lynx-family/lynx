# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
from common import *

class BaseGenerator:
    def before_generate(self):
        pass

    def after_generate(self):
        pass

    def before_gen_section(self, section):
        pass

    def before_gen_behavior(self, behavior, section):
        pass

    def after_gen_behavior(self):
        pass

    def on_next_sub_code(self, code, behavior, section):
        pass

    def on_next_meta_data(self, meta_data, is_last_element):
        pass

class ModuleGenerator(BaseGenerator):
    def __init__(self, base_indent = ""):
        self._code = []
        self._base_indent = base_indent

    def _append_with_indent(self, code):
        self._code.append("{0}{1}".format(self._base_indent, code))

    def _append(self, code):
        self._code.append(code)

    def get_sub_code(self, code, behavior, section):
        return code[KEY_LOW_CODE] + behavior[KEY_MID_CODE] * 100 + section[KEY_HIGH_CODE] * 10000

    def get_result(self):
        return "".join(self._code)


class GeneratorGroup(BaseGenerator):
    def __init__(self):
        self._child_generators = []

    def _register_child_generator(self, generator):
        self._child_generators.append(generator)

    def write(self):
        pass

    def before_generate(self):
        for c in self._child_generators:
            c.before_generate()

    def after_generate(self):
        for c in self._child_generators:
            c.after_generate()

    def before_gen_section(self, section):
        for c in self._child_generators:
            c.before_gen_section(section)

    def before_gen_behavior(self, behavior, section):
        for c in self._child_generators:
            c.before_gen_behavior(behavior, section)

    def after_gen_behavior(self):
        for c in self._child_generators:
            c.after_gen_behavior()

    def on_next_meta_data(self, meta_data, is_last_element):
        for c in self._child_generators:
            c.on_next_meta_data(meta_data, is_last_element)

    def on_next_sub_code(self, code, behavior, section):
        for c in self._child_generators:
            c.on_next_sub_code(code, behavior, section)

class PlatformGenerator(GeneratorGroup):
    def write(self):
        for c in self._child_generators:
            c.write()

class FileGenerator(GeneratorGroup):
    def __init__(self, relative_path, file_name):
        super().__init__()
        self._relative_path = relative_path
        self._file_name = file_name
        self._file_content = []
        self._format_off = CLANG_OFF
        self._format_on = CLANG_ON

    def write(self):
        current_path = os.path.dirname(os.path.abspath(__file__))
        project_root_path = os.path.join(current_path, "../../../")
        parent_path = os.path.join(project_root_path, self._relative_path)
        if not os.path.exists(parent_path):
            os.makedirs(parent_path)
        file_path = os.path.join(
            parent_path, self._file_name)
        with open(file_path, 'w') as f:
            f.write(''.join(self._file_content))
        print("generate file: {0}".format(file_path))

    def _generate_file_header(self):
        pass

    def _generate_file_footer(self):
        pass

    def _append(self, code):
        self._file_content.append(code)

    def before_generate(self):
        self._file_content.append(ALERT_COMMENT)
        self._file_content.append(self._format_off)
        self._generate_file_header()
        super().before_generate()

    def after_generate(self):
        for c in self._child_generators:
            c.after_generate()
            self._file_content.append(c.get_result())
        self._generate_file_footer()
        self._file_content.append(self._format_on)
        self._file_content.append(ALERT_COMMENT)
        self._file_content.append("\n")

class BehaviorGenerator(ModuleGenerator):
    def before_gen_section(self, section):
        # generate comment for section
        comment = SECTION_COMMENT_TEMPLATE.format(
                    self._base_indent, 
                    section[KEY_NAME], 
                    section[KEY_HIGH_CODE], 
                    section[KEY_DESC])
        self._append(comment)

    def before_gen_behavior(self, behavior, section):
        # generate comment for behavior
        comment = BEHAVIOR_COMMENT_TEMPLATE.format(
                    self._base_indent, behavior[KEY_NAME], behavior[KEY_DESC])
        self._append(comment)

    def _get_behavior_code(self, behavior, section):
        return behavior[KEY_MID_CODE] + section[KEY_HIGH_CODE] * 100

class SubCodeGenerator(BehaviorGenerator):
    def before_gen_behavior(self, behavior, section):
        super().before_gen_behavior(behavior, section)
        behavior_code = self._get_behavior_code(behavior, section)
        self._append(
            BEHAVIOR_CODE_COMMENT_TEMPLATE.format(self._base_indent, behavior_code))
        
    def after_gen_behavior(self):
        self._append("\n")

    def on_next_sub_code(self, code, behavior, section):
        # generate comment for sub code
        self._append(SUB_CODE_COMMENT_TEMPLATE.format(
            self._base_indent, code[KEY_DESC]))

class MetaDataEnumGenerator(ModuleGenerator):
    def before_generate(self):
        self._append_with_indent(META_DATA_DEF_BEGIN)

    def after_generate(self):
        self._append_with_indent(META_DATA_DEF_END)
        self._append("\n")

    