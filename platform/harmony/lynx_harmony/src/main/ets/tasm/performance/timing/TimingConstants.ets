// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.


export class TimingConstants {
  // Performance Timing Key
  public static readonly CREATE_LYNX_START = 'createLynxStart';
  public static readonly CREATE_LYNX_END = 'createLynxEnd';
  public static readonly PIPELINE_START = "pipelineStart";
  public static readonly LOAD_BUNDLE_START = "loadBundleStart";
  public static readonly RELOAD_BUNDLE_START = "reloadBundleStart";
  public static readonly VERIFY_TASM_START = "verifyTasmStart";
  public static readonly VERIFY_TASM_END = "verifyTasmEnd";
  public static readonly FFI_START = "ffiStart";
  // PipelineOrigin
  public static readonly LOAD_BUNDLE = "loadBundle";
  public static readonly RELOAD_BUNDLE_FROM_NATIVE = "reloadBundleFromNative";
  // Performance API Key
  public static readonly PIPELINE_ORIGIN = "pipelineOrigin";
  public static readonly TIMESTAMP_MAP = "timestampMap";
}