// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior;

public interface PropertiesDispatcher {
  /**
   * The bytecode of this method will be modified during compilation.
   * Do not rewrite this method or modify this method.
   * @param map
   */
  void dispatchProperties(StylesDiffMap map);
}
