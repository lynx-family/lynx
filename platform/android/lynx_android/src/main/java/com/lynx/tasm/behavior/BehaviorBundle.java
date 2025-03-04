// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import java.util.List;

public interface BehaviorBundle {
  /**
   * @return a list of view managers that will be registered int TAEnvironment
   */
  List<Behavior> create();
}
