// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.base;

import static org.mockito.Mockito.*;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import org.junit.Before;
import org.junit.Test;

public class TraceControllerTest {
  private TraceController controller;
  private TraceController.CompleteCallback callback;

  @Before
  public void setUp() throws Exception {
    controller = mock(TraceController.class);
    callback = mock(TraceController.CompleteCallback.class);
    // Initialize callback storage without invoking the native singleton constructor.
    Field callbacks = TraceController.class.getDeclaredField("mCompleteCallbacks");
    callbacks.setAccessible(true);
    callbacks.set(controller, new ArrayList<TraceController.CompleteCallback>());
    doCallRealMethod()
        .when(controller)
        .startTracing(any(TraceController.CompleteCallback.class), anyMap());
    doCallRealMethod().when(controller).onTracingComplete(anyString());
  }

  @Test
  public void startTracing() {
    Map<String, String> config = new HashMap<>();
    config.put("trace_file", "trace.proto");
    config.put("buffer_size", "1024");
    config.put("enable_systrace", "true");
    config.put("enable_compress", "true");
    config.put("enable_memory_trace", "true");
    controller.startTracing(callback, config);

    verify(controller).startTracing(1024, null, null, "trace.proto", true, true, true);
    controller.onTracingComplete("trace.proto");
    controller.onTracingComplete("trace.proto");
    verify(callback, times(1)).onComplete("trace.proto");
  }

  @Test
  public void startTracingWithDefaultConfig() {
    controller.startTracing(callback, Collections.emptyMap());

    verify(controller).startTracing(40960, null, null, "", false, false, false);
    String resolvedPath = "/data/files/trace/lynx-profile-trace-\u5e94\u7528-\uD83D\uDE80";
    controller.onTracingComplete(resolvedPath);
    controller.onTracingComplete(resolvedPath);
    verify(callback, times(1)).onComplete(resolvedPath);
  }
}
