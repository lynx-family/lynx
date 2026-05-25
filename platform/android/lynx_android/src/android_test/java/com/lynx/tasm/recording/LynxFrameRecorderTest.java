// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.recording;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import org.json.JSONArray;
import org.json.JSONObject;
import org.junit.Test;

public class LynxFrameRecorderTest {
  private static final AtomicInteger sInstanceIds = new AtomicInteger(900000);

  @Test
  public void pendingUIOperationsBeforeInitialTreeAreIgnored() throws Exception {
    int instanceId = sInstanceIds.incrementAndGet();
    LynxFrameRecorder recorder = LynxFrameRecorder.inst();
    CountDownLatch frameLatch = new CountDownLatch(2);
    List<JSONObject> frames = Collections.synchronizedList(new ArrayList<>());
    recorder.setFrameCallback(instanceId, frame -> {
      frames.add(frame);
      frameLatch.countDown();
    });

    try {
      recorder.startRecording(instanceId);
      recorder.recordCreateNode(instanceId, 1, "old-view", null, null, false, 1);
      recorder.recordInitialTree(instanceId, new int[] {2}, new String[] {"view"},
          new Object[] {null}, new Object[] {null}, new boolean[] {false}, new int[] {2},
          new int[] {-1}, new int[] {-1}, new float[25], new boolean[] {false},
          new boolean[] {false});
      recorder.recordUpdateLayout(
          instanceId, 3, 0, 0, 10, 10, new float[4], new float[4], new float[4], null, null, 0, 3);
      recorder.stopRecording(instanceId);

      assertTrue(frameLatch.await(5, TimeUnit.SECONDS));
      assertFalse(containsOperationSign(frames, 1));
      assertTrue(containsOperationSign(frames, 2));
      assertTrue(containsOperationSign(frames, 3));
    } finally {
      recorder.clearFrameCallback(instanceId);
      recorder.clearFrames(instanceId);
    }
  }

  private static boolean containsOperationSign(List<JSONObject> frames, int sign) throws Exception {
    synchronized (frames) {
      for (JSONObject frame : frames) {
        JSONObject event = frame.getJSONObject("event");
        JSONObject data = event.optJSONObject("data");
        if (data == null) {
          continue;
        }
        JSONArray operations = data.optJSONArray("operations");
        if (operations == null) {
          continue;
        }
        for (int operationIndex = 0; operationIndex < operations.length(); ++operationIndex) {
          JSONObject payload = operations.getJSONObject(operationIndex).optJSONObject("data");
          if (payload != null && payload.optInt("sign", Integer.MIN_VALUE) == sign) {
            return true;
          }
        }
      }
    }
    return false;
  }
}
