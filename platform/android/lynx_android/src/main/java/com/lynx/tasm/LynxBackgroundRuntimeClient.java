// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

public abstract class LynxBackgroundRuntimeClient {
  // Invoked on the UI thread.
  public void onReceivedError(LynxError error) {}
  // Invoked on the UI thread.
  public void onModuleMethodInvoked(String module, String method, int error_code) {}
  // Invoked on the UI thread.
  public void onEvaluateJavaScriptEnd(String url) {}
}
