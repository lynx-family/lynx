// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior;

public interface ForegroundListener {
  // The timing is when lynxView is visible
  void onLynxViewEnterForeground();
  // The timing is when lynxView is invisible
  void onLynxViewEnterBackground();
}
