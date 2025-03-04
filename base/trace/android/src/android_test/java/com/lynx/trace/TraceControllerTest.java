// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.trace;

import static org.mockito.Mockito.*;

import android.content.Context;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import java.util.HashMap;
import java.util.Map;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class TraceControllerTest {
  @Before
  public void setUp() {
    Context context =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    TraceController.getInstance().init(context);
  }

  @Test
  public void startTracing() {
    TraceController controller = TraceController.getInstance();
    TraceController spyController = spy(controller);
    Map<String, String> config = new HashMap<>();
    config.put("trace_file", "trace.proto");
    config.put("buffer_size", "1024");
    config.put("enable_systrace", "true");
    TraceController.CompleteCallback callback = new TraceController.CompleteCallback() {
      @Override
      public void onComplete(String traceFile) {}
    };
    spyController.startTracing(callback, config);

    verify(spyController).startTracing(1024, null, null, "trace.proto", true);
  }
}
