// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge;

import android.content.Context;
import androidx.annotation.NonNull;
import java.lang.ref.WeakReference;

/**
 * IContextFinder is used to find LynxContext by instanceId in {@link LynxModuleFactory} & {@link
 * LynxModuleWrapper} detail:
 * 1. find LynxContext by instanceId.
 * 2. register LynxContext by instanceId.
 */
public interface IContextFinder {
  @NonNull WeakReference<Context> findContext(String instanceId);
  void registerContext(String instanceId, WeakReference<Context> context);
}
