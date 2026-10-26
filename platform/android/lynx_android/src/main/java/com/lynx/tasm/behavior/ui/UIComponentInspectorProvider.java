// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui;

import androidx.annotation.RestrictTo;
import com.lynx.react.bridge.JavaOnlyMap;

@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
public interface UIComponentInspectorProvider {
  JavaOnlyMap getSnapshot();
}
