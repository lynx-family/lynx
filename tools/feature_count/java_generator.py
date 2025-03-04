# Copyright 2023 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import sys
from base import *

start_boundary = "  // ##### AUTO GENERATED SECTION START. DO NOT MODIFY THIS SECTION. #####\n"
end_boundary = "  // ##### AUTO GENERATED SECTION END. DO NOT MODIFY THIS SECTION. #####\n"
raw_file_top = """// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.featurecount;

import androidx.annotation.AnyThread;
import androidx.annotation.RestrictTo;
import com.lynx.tasm.LynxEnvKey;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.eventreport.LynxEventReporter;

@RestrictTo(RestrictTo.Scope.LIBRARY)
public class LynxFeatureCounter {
  //!!!!! See `tools/feature_count/README.md`\n"""

raw_file_bottom = """  private static volatile boolean sIsNativeLibraryLoaded = false;
  private static volatile boolean sEnable = LynxEnv.getBooleanFromExternalEnv(LynxEnvKey.ENABLE_FEATURE_COUNTER, false);
  public static void setEnable(boolean enable) {
    sEnable = enable;
  }
  /**
   * Count feature and upload later.
   * @param feature value of feature.
   */
  @AnyThread
  public static void count(int feature, int instanceId) {
    if (!sEnable) {
      return;
    }
    if (!sIsNativeLibraryLoaded) {
      sIsNativeLibraryLoaded = LynxEnv.inst().isNativeLibraryLoaded();
    }
    if (sIsNativeLibraryLoaded) {
      nativeFeatureCount(feature, instanceId);
    }
  }

  public static native void nativeFeatureCount(int feature, int instanceId);
}
"""

def generate(all_features, code_file):
  if code_file == None:
    print("java code file is not specified")
    sys.exit(1)
  os.makedirs(os.path.dirname(code_file), exist_ok=True)
  print(("java feature file: ", code_file))
  with open(code_file, 'w') as file:
    file.write(raw_file_top)
    file.write(start_boundary)
    for feature in all_features:
      if feature.language == Language.Java:
        file.write("  public static final int {0} = {1};\n".format(feature.enum, feature.value))
    file.write(end_boundary)
    file.write(raw_file_bottom)
