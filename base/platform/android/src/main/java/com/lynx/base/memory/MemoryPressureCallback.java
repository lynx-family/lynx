// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.base.memory;

/**
 * Memory pressure callback interface.
 */
@FunctionalInterface
public interface MemoryPressureCallback {
  void onPressure(@MemoryPressureLevel int pressure);
}
