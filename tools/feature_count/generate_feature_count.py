# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.


import os
import code_generator

dir_name = os.path.dirname(os.path.abspath(__file__))

all_features = code_generator.get_all_features()
native_file = os.path.abspath(os.path.join(dir_name, "../../core/services/feature_count/feature.h"))
java_file = os.path.abspath(os.path.join(dir_name, "../../platform/android/lynx_android/src/main/java/com/lynx/tasm/featurecount/LynxFeatureCounter.java"))
objc_file = os.path.abspath(os.path.join(dir_name, "../../platform/darwin/common/lynx/feature_count/LynxFeature.h"))
typescript_file = os.path.abspath(os.path.join(dir_name, "../../js_libraries/lynx-core/src/common/feature.ts"))
code_generator.generate_code(native_file, java_file, objc_file, typescript_file, all_features)
