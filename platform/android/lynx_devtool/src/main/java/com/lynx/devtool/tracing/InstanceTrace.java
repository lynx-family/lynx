// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.tracing;

public class InstanceTrace {
  private long mNativeInstanceTrace = 0;

  private InstanceTrace() {
    mNativeInstanceTrace = nativeCreateInstanceTrace();
  }

  private static class InstanceTraceLoader {
    private static final InstanceTrace INSTANCE = new InstanceTrace();
  }

  public static InstanceTrace getInstance() {
    return InstanceTraceLoader.INSTANCE;
  }

  public long getNativeInstanceTrace() {
    return mNativeInstanceTrace;
  }

  private native long nativeCreateInstanceTrace();
}
