// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.provider;

import androidx.annotation.NonNull;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.ReadableMap;

/**
 * use ResProvider instead.
 */
@Deprecated
public abstract class AbsNetworkingModuleProvider {
  public abstract void request(@NonNull final ReadableMap map, final Callback callback);
}
