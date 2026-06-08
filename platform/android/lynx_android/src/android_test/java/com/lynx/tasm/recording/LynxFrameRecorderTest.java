// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.recording;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.behavior.ui.PropBundle;
import java.lang.reflect.Method;
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
  private static final String LYNX_BYTE_REPLAY_MASK_PROP = "lynx-data-byte"
      + "replay-mask";

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
      recorder.recordCreateNode(instanceId, 1, "old-view", null, null);
      recorder.recordInitialTree(instanceId, new int[] {2}, new String[] {"view"},
          new Object[] {null}, new Object[] {null}, new int[] {-1}, new int[] {-1}, new float[25],
          new boolean[] {false}, new boolean[] {false}, null);
      recorder.recordUpdateLayout(
          instanceId, 3, 0, 0, 10, 10, new float[4], new float[4], new float[4], null, null, 0);
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

  @Test
  public void initialTreeRecordsTextExtraData() throws Exception {
    int instanceId = sInstanceIds.incrementAndGet();
    LynxFrameRecorder recorder = LynxFrameRecorder.inst();
    CountDownLatch frameLatch = new CountDownLatch(1);
    List<JSONObject> frames = Collections.synchronizedList(new ArrayList<>());
    recorder.setFrameCallback(instanceId, frame -> {
      frames.add(frame);
      frameLatch.countDown();
    });

    try {
      recorder.startRecording(instanceId);
      recorder.recordInitialTree(instanceId, new int[] {7}, new String[] {"text"},
          new Object[] {null}, new Object[] {null}, new int[] {-1}, new int[] {-1}, new float[25],
          new boolean[] {false}, new boolean[] {false}, new String[] {"hello"});

      assertTrue(frameLatch.await(5, TimeUnit.SECONDS));
      JSONObject payload =
          findOperationPayload(frames, LynxFrameRecorder.EVENT_LYNX_UI_UPDATE_EXTRA_DATA, 7);
      assertEquals("text", payload.getString("type"));
      assertEquals("hello", payload.getString("text"));
    } finally {
      recorder.stopRecording(instanceId);
      recorder.clearFrameCallback(instanceId);
      recorder.clearFrames(instanceId);
    }
  }

  @Test
  public void maskTrueMasksDescendantText() throws Exception {
    int instanceId = sInstanceIds.incrementAndGet();
    LynxFrameRecorder recorder = LynxFrameRecorder.inst();
    CountDownLatch frameLatch = new CountDownLatch(1);
    List<JSONObject> frames = Collections.synchronizedList(new ArrayList<>());
    recorder.setFrameCallback(instanceId, frame -> {
      frames.add(frame);
      frameLatch.countDown();
    });

    try {
      recorder.startRecording(instanceId);
      recorder.recordInitialTree(instanceId, new int[] {10, 11}, new String[] {"view", "text"},
          new Object[] {propBundle(LYNX_BYTE_REPLAY_MASK_PROP, "true"), null},
          new Object[] {null, null}, new int[] {-1, 10}, new int[] {-1, 0}, new float[50],
          new boolean[] {false, false}, new boolean[] {false, false},
          new String[] {null, "secret"});

      assertTrue(frameLatch.await(5, TimeUnit.SECONDS));
      JSONObject textPayload =
          findOperationPayload(frames, LynxFrameRecorder.EVENT_LYNX_UI_UPDATE_EXTRA_DATA, 11);
      assertEquals("******", textPayload.getString("text"));

      JSONObject createPayload =
          findOperationPayload(frames, LynxFrameRecorder.EVENT_LYNX_UI_CREATE_NODE, 10);
      JSONObject props = createPayload.getJSONObject("bundle").getJSONObject("props");
      assertEquals("true", props.getString(LYNX_BYTE_REPLAY_MASK_PROP));
    } finally {
      recorder.stopRecording(instanceId);
      recorder.clearFrameCallback(instanceId);
      recorder.clearFrames(instanceId);
    }
  }

  @Test
  public void maskNameMasksOnlyCurrentNodeText() throws Exception {
    int instanceId = sInstanceIds.incrementAndGet();
    LynxFrameRecorder recorder = LynxFrameRecorder.inst();
    CountDownLatch frameLatch = new CountDownLatch(1);
    List<JSONObject> frames = Collections.synchronizedList(new ArrayList<>());
    recorder.setFrameCallback(instanceId, frame -> {
      frames.add(frame);
      frameLatch.countDown();
    });

    try {
      recorder.startRecording(instanceId);
      recorder.recordInitialTree(instanceId, new int[] {20, 21}, new String[] {"text", "text"},
          new Object[] {propBundle(LYNX_BYTE_REPLAY_MASK_PROP, "name"), null},
          new Object[] {null, null}, new int[] {-1, 20}, new int[] {-1, 0}, new float[50],
          new boolean[] {false, false}, new boolean[] {false, false},
          new String[] {"root-secret", "child-secret"});

      assertTrue(frameLatch.await(5, TimeUnit.SECONDS));
      JSONObject rootText =
          findOperationPayload(frames, LynxFrameRecorder.EVENT_LYNX_UI_UPDATE_EXTRA_DATA, 20);
      JSONObject childText =
          findOperationPayload(frames, LynxFrameRecorder.EVENT_LYNX_UI_UPDATE_EXTRA_DATA, 21);
      assertEquals("***********", rootText.getString("text"));
      assertEquals("child-secret", childText.getString("text"));

      JSONObject createPayload =
          findOperationPayload(frames, LynxFrameRecorder.EVENT_LYNX_UI_CREATE_NODE, 20);
      JSONObject props = createPayload.getJSONObject("bundle").getJSONObject("props");
      assertEquals("name", props.getString(LYNX_BYTE_REPLAY_MASK_PROP));
    } finally {
      recorder.stopRecording(instanceId);
      recorder.clearFrameCallback(instanceId);
      recorder.clearFrames(instanceId);
    }
  }

  @Test
  public void maskIgnoreKeepsOnlyRootBox() throws Exception {
    int instanceId = sInstanceIds.incrementAndGet();
    LynxFrameRecorder recorder = LynxFrameRecorder.inst();
    CountDownLatch frameLatch = new CountDownLatch(1);
    List<JSONObject> frames = Collections.synchronizedList(new ArrayList<>());
    recorder.setFrameCallback(instanceId, frame -> {
      frames.add(frame);
      frameLatch.countDown();
    });

    try {
      recorder.startRecording(instanceId);
      recorder.recordInitialTree(instanceId, new int[] {30, 31}, new String[] {"text", "text"},
          new Object[] {propBundle(LYNX_BYTE_REPLAY_MASK_PROP, "ignore", "class", "masked-box",
                            "value", "root-secret", "src", "https://example.com/private.png"),
              null},
          new Object[] {null, null}, new int[] {-1, 30}, new int[] {-1, 0}, new float[50],
          new boolean[] {false, false}, new boolean[] {false, false},
          new String[] {"root-secret", "child-secret"});

      assertTrue(frameLatch.await(5, TimeUnit.SECONDS));
      JSONObject createPayload =
          findOperationPayload(frames, LynxFrameRecorder.EVENT_LYNX_UI_CREATE_NODE, 30);
      assertEquals("view", createPayload.getString("tagName"));
      JSONObject props = createPayload.getJSONObject("bundle").getJSONObject("props");
      assertEquals("ignore", props.getString(LYNX_BYTE_REPLAY_MASK_PROP));
      assertEquals("masked-box", props.getString("class"));
      assertEquals(mask("root-secret"), props.getString("value"));
      assertEquals(mask("https://example.com/private.png"), props.getString("src"));
      assertTrue(containsOperation(frames, LynxFrameRecorder.EVENT_LYNX_UI_UPDATE_LAYOUT, 30));
      assertFalse(containsOperation(frames, LynxFrameRecorder.EVENT_LYNX_UI_UPDATE_PROPS, 30));
      assertFalse(containsOperationSign(frames, 31));
    } finally {
      recorder.stopRecording(instanceId);
      recorder.clearFrameCallback(instanceId);
      recorder.clearFrames(instanceId);
    }
  }

  @Test
  public void maskStrictMasksTextAndFiltersMedia() throws Exception {
    int instanceId = sInstanceIds.incrementAndGet();
    LynxFrameRecorder recorder = LynxFrameRecorder.inst();
    CountDownLatch frameLatch = new CountDownLatch(1);
    List<JSONObject> frames = Collections.synchronizedList(new ArrayList<>());
    recorder.setFrameCallback(instanceId, frame -> {
      frames.add(frame);
      frameLatch.countDown();
    });

    try {
      recorder.startRecording(instanceId);
      recorder.recordInitialTree(instanceId, new int[] {40, 41, 42},
          new String[] {"view", "text", "image"},
          new Object[] {propBundle(LYNX_BYTE_REPLAY_MASK_PROP, "strict"), null,
              propBundle("src", "https://example.com/private.png", "mode", "aspectFit", "value",
                  "secret-media")},
          new Object[] {null, null, null}, new int[] {-1, 40, 40}, new int[] {-1, 0, 1},
          new float[75], new boolean[] {false, false, false}, new boolean[] {false, false, false},
          new String[] {null, "secret", null});

      assertTrue(frameLatch.await(5, TimeUnit.SECONDS));
      JSONObject textPayload =
          findOperationPayload(frames, LynxFrameRecorder.EVENT_LYNX_UI_UPDATE_EXTRA_DATA, 41);
      assertEquals("******", textPayload.getString("text"));

      JSONObject rootCreate =
          findOperationPayload(frames, LynxFrameRecorder.EVENT_LYNX_UI_CREATE_NODE, 40);
      JSONObject props = rootCreate.getJSONObject("bundle").getJSONObject("props");
      assertEquals("strict", props.getString(LYNX_BYTE_REPLAY_MASK_PROP));

      JSONObject imageCreate =
          findOperationPayload(frames, LynxFrameRecorder.EVENT_LYNX_UI_CREATE_NODE, 42);
      assertEquals("view", imageCreate.getString("tagName"));
      JSONObject imageProps = imageCreate.getJSONObject("bundle").getJSONObject("props");
      assertEquals("aspectFit", imageProps.getString("mode"));
      assertEquals(mask("https://example.com/private.png"), imageProps.getString("src"));
      assertEquals(mask("secret-media"), imageProps.getString("value"));
      assertFalse(containsOperation(frames, LynxFrameRecorder.EVENT_LYNX_UI_UPDATE_PROPS, 42));
      assertTrue(containsOperation(frames, LynxFrameRecorder.EVENT_LYNX_UI_UPDATE_LAYOUT, 42));
    } finally {
      recorder.stopRecording(instanceId);
      recorder.clearFrameCallback(instanceId);
      recorder.clearFrames(instanceId);
    }
  }

  @Test
  public void maskIgnoreBoxRecordsSanitizedPropUpdates() throws Exception {
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
      recorder.recordInitialTree(instanceId, new int[] {45}, new String[] {"view"},
          new Object[] {propBundle(LYNX_BYTE_REPLAY_MASK_PROP, "ignore", "class", "initial")},
          new Object[] {null}, new int[] {-1}, new int[] {-1}, new float[25], new boolean[] {false},
          new boolean[] {false}, null);
      recorder.recordUpdateProps(instanceId, 45, false,
          propBundle(
              "class", "updated", "value", "next-secret", "src", "https://example.com/updated.png"),
          null);
      recorder.stopRecording(instanceId);

      assertTrue(frameLatch.await(5, TimeUnit.SECONDS));
      JSONObject updateProps =
          findOperationPayload(frames, LynxFrameRecorder.EVENT_LYNX_UI_UPDATE_PROPS, 45);
      JSONObject props = updateProps.getJSONObject("bundle").getJSONObject("props");
      assertEquals("updated", props.getString("class"));
      assertEquals(mask("next-secret"), props.getString("value"));
      assertEquals(mask("https://example.com/updated.png"), props.getString("src"));
    } finally {
      recorder.clearFrameCallback(instanceId);
      recorder.clearFrames(instanceId);
    }
  }

  @Test
  public void passwordInputDoesNotRecordInputValue() throws Exception {
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
      recorder.recordInitialTree(instanceId, new int[] {50}, new String[] {"input"},
          new Object[] {propBundle("type", "password", "value", "secret")}, new Object[] {null},
          new int[] {-1}, new int[] {-1}, new float[25], new boolean[] {false},
          new boolean[] {false}, null);
      JavaOnlyMap params = new JavaOnlyMap();
      params.putString("value", "next-secret");
      params.putInt("cursor", 1);
      recorder.recordInvoke(instanceId, 50, "setValue", params);
      recorder.recordInputValue(instanceId, 50, "typed-secret", 2, 2);
      recorder.stopRecording(instanceId);

      assertTrue(frameLatch.await(5, TimeUnit.SECONDS));
      JSONObject updateProps =
          findOperationPayload(frames, LynxFrameRecorder.EVENT_LYNX_UI_UPDATE_PROPS, 50);
      JSONObject props = updateProps.getJSONObject("bundle").getJSONObject("props");
      assertEquals("******", props.getString("value"));
      assertEquals("password", props.getString("type"));

      JSONObject invokePayload =
          findOperationPayload(frames, LynxFrameRecorder.EVENT_LYNX_UI_INVOKE, 50);
      JSONObject invokeParams = invokePayload.getJSONObject("params");
      assertEquals("***********", invokeParams.getString("value"));
      assertEquals(1, invokeParams.getInt("cursor"));

      JSONObject inputValuePayload =
          findOperationPayload(frames, LynxFrameRecorder.EVENT_LYNX_UI_UPDATE_INPUT_VALUE, 50);
      assertEquals("************", inputValuePayload.getString("value"));
      assertEquals(2, inputValuePayload.getInt("selectionStart"));
      assertEquals(2, inputValuePayload.getInt("selectionEnd"));
    } finally {
      recorder.clearFrameCallback(instanceId);
      recorder.clearFrames(instanceId);
    }
  }

  @Test
  public void inputValueRecordsTextAndSelection() throws Exception {
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
      recorder.recordInitialTree(instanceId, new int[] {51}, new String[] {"input"},
          new Object[] {propBundle("type", "text")}, new Object[] {null}, new int[] {-1},
          new int[] {-1}, new float[25], new boolean[] {false}, new boolean[] {false}, null);
      recorder.recordInputValue(instanceId, 51, "hello", 5, 5);
      recorder.stopRecording(instanceId);

      assertTrue(frameLatch.await(5, TimeUnit.SECONDS));
      JSONObject inputValuePayload =
          findOperationPayload(frames, LynxFrameRecorder.EVENT_LYNX_UI_UPDATE_INPUT_VALUE, 51);
      assertEquals("hello", inputValuePayload.getString("value"));
      assertEquals(5, inputValuePayload.getInt("selectionStart"));
      assertEquals(5, inputValuePayload.getInt("selectionEnd"));
    } finally {
      recorder.clearFrameCallback(instanceId);
      recorder.clearFrames(instanceId);
    }
  }

  @Test
  public void ignoredSubtreeTouchTargetMapsToVisibleBox() throws Exception {
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
      recorder.recordInitialTree(instanceId, new int[] {60, 61}, new String[] {"view", "text"},
          new Object[] {propBundle(LYNX_BYTE_REPLAY_MASK_PROP, "ignore"), null},
          new Object[] {null, null}, new int[] {-1, 60}, new int[] {-1, 0}, new float[50],
          new boolean[] {false, false}, new boolean[] {false, false},
          new String[] {null, "secret"});
      List<LynxFrameRecorder.TouchPointPayload> points = new ArrayList<>();
      points.add(new LynxFrameRecorder.TouchPointPayload(1, 2, 0, 61));
      recorder.recordTouch(instanceId, LynxFrameRecorder.EVENT_LYNX_TOUCH_START, points, 123);

      assertTrue(frameLatch.await(5, TimeUnit.SECONDS));
      assertEquals(60, findFirstTouchTarget(frames));
    } finally {
      recorder.stopRecording(instanceId);
      recorder.clearFrameCallback(instanceId);
      recorder.clearFrames(instanceId);
    }
  }

  @Test
  public void uiOperationPayloadOmitsUnusedFields() throws Exception {
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
      recorder.recordInitialTree(instanceId, new int[] {8}, new String[] {"view"},
          new Object[] {null}, new Object[] {null}, new int[] {-1}, new int[] {-1}, new float[25],
          new boolean[] {false}, new boolean[] {false}, null);
      recorder.recordInvoke(instanceId, 8, "noop", null);
      recorder.stopRecording(instanceId);

      assertTrue(frameLatch.await(5, TimeUnit.SECONDS));
      JSONObject createPayload =
          findOperationPayload(frames, LynxFrameRecorder.EVENT_LYNX_UI_CREATE_NODE, 8);
      assertFalse(createPayload.has("isFlatten"));
      assertFalse(createPayload.has("nodeIndex"));

      JSONObject layoutPayload =
          findOperationPayload(frames, LynxFrameRecorder.EVENT_LYNX_UI_UPDATE_LAYOUT, 8);
      assertFalse(layoutPayload.has("nodeIndex"));

      JSONObject updatePropsPayload =
          findOperationPayload(frames, LynxFrameRecorder.EVENT_LYNX_UI_UPDATE_PROPS, 8);
      assertFalse(updatePropsPayload.has("tendToFlatten"));

      JSONObject invokePayload =
          findOperationPayload(frames, LynxFrameRecorder.EVENT_LYNX_UI_INVOKE, 8);
      assertFalse(invokePayload.has("context"));
      assertFalse(invokePayload.has("callback"));
    } finally {
      recorder.clearFrameCallback(instanceId);
      recorder.clearFrames(instanceId);
    }
  }

  @Test
  public void uiFramePayloadIncludesScreenDensity() throws Exception {
    int instanceId = sInstanceIds.incrementAndGet();
    LynxFrameRecorder recorder = LynxFrameRecorder.inst();
    CountDownLatch frameLatch = new CountDownLatch(1);
    List<JSONObject> frames = Collections.synchronizedList(new ArrayList<>());
    recorder.setFrameCallback(instanceId, frame -> {
      frames.add(frame);
      frameLatch.countDown();
    });

    try {
      recorder.startRecording(instanceId, 3.f);
      recorder.recordInitialTree(instanceId, new int[] {9}, new String[] {"view"},
          new Object[] {null}, new Object[] {null}, new int[] {-1}, new int[] {-1}, new float[25],
          new boolean[] {false}, new boolean[] {false}, null);

      assertTrue(frameLatch.await(5, TimeUnit.SECONDS));
      JSONObject data = frames.get(0).getJSONObject("event").getJSONObject("data");
      assertEquals(3.0, data.getDouble("screenDensity"), 0.001);
    } finally {
      recorder.stopRecording(instanceId);
      recorder.clearFrameCallback(instanceId);
      recorder.clearFrames(instanceId);
    }
  }

  private static PropBundle propBundle(String... keyValues) throws Exception {
    Method create = PropBundle.class.getDeclaredMethod("createPropBundle");
    create.setAccessible(true);
    PropBundle bundle = (PropBundle) create.invoke(null);
    for (int i = 0; i + 1 < keyValues.length; i += 2) {
      bundle.putString(keyValues[i], keyValues[i + 1]);
    }
    return bundle;
  }

  private static String mask(String value) {
    int length = value.codePointCount(0, value.length());
    StringBuilder result = new StringBuilder(length);
    for (int index = 0; index < length; ++index) {
      result.append("*");
    }
    return result.toString();
  }

  private static boolean containsOperation(List<JSONObject> frames, int eventType, int sign)
      throws Exception {
    try {
      findOperationPayload(frames, eventType, sign);
      return true;
    } catch (AssertionError error) {
      return false;
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

  private static JSONObject findOperationPayload(List<JSONObject> frames, int eventType, int sign)
      throws Exception {
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
          JSONObject operation = operations.getJSONObject(operationIndex);
          if (operation.optInt("eventType", Integer.MIN_VALUE) != eventType) {
            continue;
          }
          JSONObject payload = operation.optJSONObject("data");
          if (payload != null && payload.optInt("sign", Integer.MIN_VALUE) == sign) {
            return payload;
          }
        }
      }
    }
    throw new AssertionError("operation not found, eventType=" + eventType + ", sign=" + sign);
  }

  private static int findFirstTouchTarget(List<JSONObject> frames) throws Exception {
    synchronized (frames) {
      for (JSONObject frame : frames) {
        JSONObject event = frame.getJSONObject("event");
        if (event.optInt("eventType", Integer.MIN_VALUE)
            != LynxFrameRecorder.EVENT_LYNX_TOUCH_START) {
          continue;
        }
        JSONArray points = event.getJSONObject("data").getJSONArray("points");
        return points.getJSONObject(0).getInt("targetId");
      }
    }
    throw new AssertionError("touch frame not found");
  }
}
