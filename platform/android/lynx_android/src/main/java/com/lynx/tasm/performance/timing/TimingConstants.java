// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.timing;
public class TimingConstants {
  // Performance Timing Key
  public static final String PIPELINE_START = "pipelineStart";
  public static final String LOAD_BUNDLE_START = "loadBundleStart";
  public static final String VERIFY_TASM_START = "verifyTasmStart";
  public static final String VERIFY_TASM_END = "verifyTasmEnd";
  public static final String FFI_START = "ffiStart";
  // PipelineOrigin
  public static final String LOAD_BUNDLE = "loadBundle";
  public static final String RELOAD_BUNDLE_FROM_NATIVE = "reloadBundleFromNative";
  // Performance API Key
  public static final String PIPELINE_ORIGIN = "pipelineOrigin";
  public static final String TIMESTAMP_MAP = "timestampMap";
}
