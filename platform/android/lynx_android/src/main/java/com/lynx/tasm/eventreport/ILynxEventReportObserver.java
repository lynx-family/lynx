// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.eventreport;

import androidx.annotation.AnyThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.util.Map;

public interface ILynxEventReportObserver {
  /**
   * Callback method to listen and handle Lynx reporting events.
   * Called in report thread.
   * @param eventName name of event.
   * @param instanceId the unique id of lynx template instance.
   * @param props all properties of event. This's an unmodifiable Map, modification operation will
   *     throw an `UnsupportedOperationException` exception.
   * @param extraData parameters set by method `LynxView.putParamsForReportingEvents()` of report
   *     event. This's an unmodifiable Map, modification operation will throw an
   * `UnsupportedOperationException` exception.
   */
  void onReportEvent(@NonNull String eventName, int instanceId,
      @NonNull Map<String, ? extends Object> props,
      @Nullable Map<String, ? extends Object> extraData);
}
