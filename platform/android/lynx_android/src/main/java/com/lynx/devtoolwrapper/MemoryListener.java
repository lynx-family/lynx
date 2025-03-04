// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtoolwrapper;

import androidx.annotation.AnyThread;
import androidx.annotation.RestrictTo;
import java.util.ArrayList;
import java.util.concurrent.CopyOnWriteArrayList;
import org.json.JSONObject;

public class MemoryListener {
  // Need to be thread-safe
  private CopyOnWriteArrayList<MemoryReporter> mMemoryReporters;

  private static class MemoryListenerLoader {
    private static final MemoryListener INSTANCE = new MemoryListener();
  }

  private MemoryListener() {
    mMemoryReporters = new CopyOnWriteArrayList<>();
  }

  public static MemoryListener getInstance() {
    return MemoryListenerLoader.INSTANCE;
  }

  public void uploadImageInfo(JSONObject data) {
    for (MemoryReporter reporter : mMemoryReporters) {
      reporter.uploadImageInfo(data);
    }
  }

  public void addMemoryReporter(MemoryReporter reporter) {
    mMemoryReporters.add(reporter);
  }

  public void removeMemoryReporter(MemoryReporter reporter) {
    mMemoryReporters.remove(reporter);
  }

  /**
   * Get whether any available reporter registered
   * @return true if there is at least one available reporter, otherwise false
   */
  @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
  @AnyThread
  public boolean hasAvailableReporter() {
    return !mMemoryReporters.isEmpty();
  }

  public interface MemoryReporter {
    // upload image fetch info to devtool
    void uploadImageInfo(JSONObject data);
  }
}
