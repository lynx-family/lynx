// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.ref;

import androidx.annotation.AnyThread;

public abstract class ResourceReleaser<T> {
  @AnyThread protected abstract void release(T t);
}
