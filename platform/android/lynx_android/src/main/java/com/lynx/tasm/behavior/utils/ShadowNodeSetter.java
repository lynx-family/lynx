// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.utils;

import com.lynx.tasm.behavior.StylesDiffMap;
import com.lynx.tasm.behavior.shadow.ShadowNode;

public interface ShadowNodeSetter<T extends ShadowNode> extends Settable {
  void setProperty(ShadowNode node, String name, StylesDiffMap props);
}
