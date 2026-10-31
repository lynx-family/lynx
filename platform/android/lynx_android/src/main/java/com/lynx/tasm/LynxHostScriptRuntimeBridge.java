// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

/** Private installation hook implemented by the matching engine. */
final class LynxHostScriptRuntimeBridge {
  private LynxHostScriptRuntimeBridge() {}

  static native boolean nativeSetRuntimeCallbacks(long callbacks, boolean commit);
}
