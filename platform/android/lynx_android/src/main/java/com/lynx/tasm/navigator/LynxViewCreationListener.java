// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.navigator;

import com.lynx.tasm.LynxView;

public interface LynxViewCreationListener {
  void onReady(LynxView lynxView);

  void onFailed();
}
