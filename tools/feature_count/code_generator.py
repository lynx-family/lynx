# Copyright 2023 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import base
import native_generator
import java_generator
import typescript_generator
import objc_generator

def get_all_features(spec_files=None):
    dir_name = os.path.dirname(os.path.abspath(__file__))
    if spec_files is None:
        spec_files = [os.path.join(dir_name, "specification.yaml")]
    all_features = base.get_all_features(spec_files[0])
    for spec_file in spec_files[1:]:
        all_features = base.merge_to_new_features(
            all_features, base.get_all_features_from_spec(spec_file))
    return all_features

def generate_code(native_file, java_file, objc_file, typescript_file, all_features, lang=None):
    generators = {
        "cpp": (native_generator, native_file),
        "java": (java_generator, java_file),
        "objc": (objc_generator, objc_file),
        "typescript": (typescript_generator, typescript_file),
    }
    for language, (generator, output_file) in generators.items():
        if lang is None or language in lang:
            generator.generate(all_features, output_file)
