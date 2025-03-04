// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.utils;

import com.lynx.tasm.behavior.StylesDiffMap;
import com.lynx.tasm.behavior.ui.LynxBaseUI;

public interface LynxUISetter<T extends LynxBaseUI> extends Settable {
  void setProperty(LynxBaseUI ui, String name, StylesDiffMap props);
}
