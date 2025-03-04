# Copyright 2023 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import base
import native_generator
import java_generator
import typescript_generator
import objc_generator

def get_all_features():
    dir_name = os.path.dirname(os.path.abspath(__file__))
    spec_file = os.path.join(dir_name, "specification.yaml")
    all_features = base.get_all_features(spec_file)
    return all_features

def generate_code(native_file, java_file, objc_file, typescript_file, all_features):
    native_generator.generate(all_features, native_file)
    java_generator.generate(all_features, java_file)
    objc_generator.generate(all_features, objc_file)
    typescript_generator.generate(all_features, typescript_file)
