// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.base;

public interface IComplicatedLogDelegate extends ILogDelegate {
  boolean getShouldFormatMessage();
  boolean isComplicatedLogLoggable(int level, LogSource source, Long runtimeId);
}
