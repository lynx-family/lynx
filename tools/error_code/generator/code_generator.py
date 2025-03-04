# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import sys
from common import *
from generator.android.android_generator import *
from generator.darwin.darwin_generator import *
from generator.native.native_generator import *
from generator.typescript.ts_generator import *

__all__ = ['ErrorCodeGenerator']

class ErrorCodeGenerator:
    def __init__(self, spec, lang):
        if lang == None:
            lang = [LANG_TS, LANG_OC, LANG_CPP, LANG_JAVA]
        self._spec = spec
        self._child_generators = []
        meta_data_list = spec[KEY_METADATA]
        # register child generators
        if LANG_JAVA in lang:
            self._child_generators.append(AndroidGenerator(meta_data_list))
        if LANG_OC in lang:
            self._child_generators.append(DarwinGenerator(meta_data_list))
        if LANG_CPP in lang:
            self._child_generators.append(NativeGenerator())
        if LANG_TS in lang:
            self._child_generators.append(TypescriptGenerator())

    def generate(self):
        for c in self._child_generators:
            c.before_generate()
        self._traverse_meta_data_list(self._spec[KEY_METADATA])
        self._traverse_sections(self._spec[KEY_SECTIONS])
        for c in self._child_generators:
            c.after_generate()
            c.write()

    def _traverse_meta_data_list(self, meta_data_list):
        for meta_data in meta_data_list:
            is_last_element = False
            if meta_data == meta_data_list[-1]:
                is_last_element = True
            for c in self._child_generators:
                c.on_next_meta_data(meta_data, is_last_element)

    def _traverse_sections(self, sections):
        for section in sections:
            self._traverse_section(section)

    def _traverse_section(self, section):
        for c in self._child_generators:
            c.before_gen_section(section)
        for behavior in section["behaviors"]:
            self._traverse_behavior(behavior, section)

    def _traverse_behavior(self, behavior, section):
        for c in self._child_generators:
            c.before_gen_behavior(behavior, section)
        for code in behavior["codes"]:
            for c in self._child_generators:
                c.on_next_sub_code(code, behavior, section)
        for c in self._child_generators:
            c.after_gen_behavior()