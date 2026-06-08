// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.recording;

import android.os.SystemClock;
import android.view.Choreographer;
import androidx.annotation.RestrictTo;
import com.lynx.react.bridge.Dynamic;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.ReadableMapKeySetIterator;
import com.lynx.react.bridge.ReadableType;
import com.lynx.react.bridge.mapbuffer.MapBufferUtils;
import com.lynx.react.bridge.mapbuffer.ReadableMapBuffer;
import com.lynx.tasm.behavior.ui.PropBundle;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.reflect.Array;
import java.nio.ByteBuffer;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Deque;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.atomic.AtomicInteger;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * @hide
 */
@RestrictTo(RestrictTo.Scope.LIBRARY)
public final class LynxFrameRecorder {
  public static final int TRACK_TYPE_LYNX_UI_OP = 100;
  public static final int TRACK_TYPE_LYNX_TOUCH = 200;

  public static final int EVENT_LYNX_UI_FRAME = 101;
  public static final int EVENT_LYNX_UI_CREATE_NODE = 102;
  public static final int EVENT_LYNX_UI_INSERT_NODE = 103;
  public static final int EVENT_LYNX_UI_REMOVE_NODE = 104;
  public static final int EVENT_LYNX_UI_DESTROY_NODE = 105;
  public static final int EVENT_LYNX_UI_UPDATE_PROPS = 106;
  public static final int EVENT_LYNX_UI_UPDATE_LAYOUT = 107;
  public static final int EVENT_LYNX_UI_INVOKE = 108;
  public static final int EVENT_LYNX_UI_UPDATE_EXTRA_DATA = 109;
  public static final int EVENT_LYNX_UI_UPDATE_INPUT_VALUE = 110;

  public static final int EVENT_LYNX_TOUCH_START = 201;
  public static final int EVENT_LYNX_TOUCH_MOVE = 202;
  public static final int EVENT_LYNX_TOUCH_END = 203;
  public static final int EVENT_LYNX_TOUCH_CANCEL = 204;

  private static final int MAX_CACHED_FRAMES_PER_INSTANCE = 1024;
  private static final int INITIAL_TREE_LAYOUT_STRIDE = 25;
  private static final int INITIAL_TREE_X_INDEX = 0;
  private static final int INITIAL_TREE_Y_INDEX = 1;
  private static final int INITIAL_TREE_WIDTH_INDEX = 2;
  private static final int INITIAL_TREE_HEIGHT_INDEX = 3;
  private static final int INITIAL_TREE_PADDING_INDEX = 4;
  private static final int INITIAL_TREE_MARGIN_INDEX = 8;
  private static final int INITIAL_TREE_BORDER_INDEX = 12;
  private static final int INITIAL_TREE_BOUNDS_INDEX = 16;
  private static final int INITIAL_TREE_STICKY_INDEX = 20;
  private static final int INITIAL_TREE_MAX_HEIGHT_INDEX = 24;
  private static final long NANOS_PER_MILLIS = 1000000L;
  private static final String LYNX_BYTE_REPLAY_MASK_PROP = "lynx-data-byte"
      + "replay-mask";
  private static final String MASK_VALUE_TRUE = "true";
  private static final String MASK_VALUE_IGNORE = "ignore";
  private static final String MASK_VALUE_NAME = "name";
  private static final String MASK_VALUE_STRICT = "strict";
  private static final String MASKED_TEXT = "*";
  private static final String TEXT_EXTRA_DATA_TYPE = "text";
  private static final String INPUT_TAG = "input";
  private static final String X_INPUT_TAG = "x-input";
  private static final String TEXTAREA_TAG = "textarea";
  private static final String X_TEXTAREA_TAG = "x-textarea";
  private static final String INPUT_TYPE_PROP = "type";
  private static final String INPUT_PASSWORD_TYPE = "password";
  private static final String VIEW_TAG = "view";

  private static final LynxFrameRecorder sInstance = new LynxFrameRecorder();

  private final ExecutorService mExecutor =
      Executors.newSingleThreadExecutor(new RecorderThreadFactory());
  private final ConcurrentHashMap<Integer, Integer> mRecordingSessions = new ConcurrentHashMap<>();
  private final ConcurrentHashMap<Integer, Integer> mStoppingSessions = new ConcurrentHashMap<>();
  private final ConcurrentHashMap<Integer, Integer> mInitialTreePendingSessions =
      new ConcurrentHashMap<>();
  private final ConcurrentHashMap<Integer, FrameCache> mFrames = new ConcurrentHashMap<>();
  private final ConcurrentHashMap<Integer, FrameCallback> mFrameCallbacks =
      new ConcurrentHashMap<>();
  private final ConcurrentHashMap<Integer, Float> mScreenDensities = new ConcurrentHashMap<>();
  private final Map<Integer, PendingUIFrame> mPendingUIFrames = new HashMap<>();
  private final Map<Integer, RecorderTreeState> mTreeStates = new HashMap<>();
  private final AtomicInteger mNextSessionId = new AtomicInteger(1);

  private LynxFrameRecorder() {}

  public static LynxFrameRecorder inst() {
    return sInstance;
  }

  public interface FrameCallback {
    void onLynxFrame(JSONObject frame);
  }

  public void setFrameCallback(int instanceId, FrameCallback callback) {
    if (callback == null) {
      mFrameCallbacks.remove(instanceId);
      return;
    }
    mFrameCallbacks.put(instanceId, callback);
  }

  public void clearFrameCallback(int instanceId) {
    mFrameCallbacks.remove(instanceId);
  }

  public void startRecording(int instanceId) {
    startRecording(instanceId, 0.f);
  }

  public void startRecording(int instanceId, float screenDensity) {
    if (instanceId < 0) {
      return;
    }
    if (screenDensity > 0.f) {
      mScreenDensities.put(instanceId, screenDensity);
    } else {
      mScreenDensities.remove(instanceId);
    }
    int sessionId = mNextSessionId.getAndIncrement();
    mInitialTreePendingSessions.put(instanceId, sessionId);
    mRecordingSessions.put(instanceId, sessionId);
    mStoppingSessions.remove(instanceId);
    mFrames.put(instanceId, new FrameCache(MAX_CACHED_FRAMES_PER_INSTANCE));
    mExecutor.execute(() -> {
      if (!isRecordingSession(instanceId, sessionId)) {
        return;
      }
      mTreeStates.put(instanceId, new RecorderTreeState(sessionId));
      clearPendingUIFrameOnExecutor(instanceId, null);
    });
  }

  public void stopRecording(int instanceId) {
    if (instanceId < 0) {
      return;
    }
    Integer sessionId = getActiveRecordingSession(instanceId);
    if (sessionId == null) {
      mFrames.remove(instanceId);
      mScreenDensities.remove(instanceId);
      mExecutor.execute(() -> mTreeStates.remove(instanceId));
      return;
    }
    mStoppingSessions.put(instanceId, sessionId);
    mExecutor.execute(() -> {
      flushPendingUIFrameOnExecutor(instanceId, sessionId, SystemClock.uptimeMillis());
      mRecordingSessions.remove(instanceId, sessionId);
      mStoppingSessions.remove(instanceId, sessionId);
      mInitialTreePendingSessions.remove(instanceId, sessionId);
      if (!mRecordingSessions.containsKey(instanceId)) {
        mFrames.remove(instanceId);
        mScreenDensities.remove(instanceId);
        mTreeStates.remove(instanceId);
      }
    });
  }

  public void recordCreateNode(int instanceId, int sign, String tagName, PropBundle bundle,
      ReadableMapBuffer initialStyles) {
    long opTimeMillis = SystemClock.uptimeMillis();
    Map<String, Object> bundleSnapshot = snapshotPropBundle(bundle);
    Map<String, Object> initialStyleSnapshot = snapshotStyleMapBuffer(initialStyles);
    recordUIOperation(instanceId, EVENT_LYNX_UI_CREATE_NODE,
        new CreateNodePayload(sign, tagName, bundleSnapshot, initialStyleSnapshot), opTimeMillis);
  }

  public void recordNodeRelation(
      int instanceId, int eventType, int parentSign, int childSign, Integer index) {
    long opTimeMillis = SystemClock.uptimeMillis();
    recordUIOperation(
        instanceId, eventType, new NodeRelationPayload(parentSign, childSign, index), opTimeMillis);
  }

  public void recordUpdateProps(int instanceId, int sign, boolean tendToFlatten, PropBundle bundle,
      ReadableMapBuffer styles) {
    long opTimeMillis = SystemClock.uptimeMillis();
    Map<String, Object> bundleSnapshot = snapshotPropBundle(bundle);
    Map<String, Object> styleSnapshot = snapshotStyleMapBuffer(styles);
    recordUIOperation(instanceId, EVENT_LYNX_UI_UPDATE_PROPS,
        new UpdatePropsPayload(sign, tendToFlatten, bundleSnapshot, styleSnapshot), opTimeMillis);
  }

  public void recordUpdateLayout(int instanceId, int sign, float x, float y, float width,
      float height, float[] paddings, float[] margins, float[] borders, float[] bounds,
      float[] sticky, float maxHeight) {
    long opTimeMillis = SystemClock.uptimeMillis();
    recordUIOperation(instanceId, EVENT_LYNX_UI_UPDATE_LAYOUT,
        new UpdateLayoutPayload(sign, x, y, width, height, copyFloatArray(paddings),
            copyFloatArray(margins), copyFloatArray(borders), copyFloatArray(bounds),
            copyFloatArray(sticky), maxHeight),
        opTimeMillis);
  }

  public void recordInvoke(int instanceId, int sign, String method, ReadableMap params) {
    long opTimeMillis = SystemClock.uptimeMillis();
    Map<String, Object> paramsSnapshot = snapshotReadableMap(params);
    recordUIOperation(instanceId, EVENT_LYNX_UI_INVOKE,
        new InvokePayload(sign, method, paramsSnapshot), opTimeMillis);
  }

  public void recordUpdateExtraData(int instanceId, int sign, String type, String text) {
    if (type == null || text == null) {
      return;
    }
    long opTimeMillis = SystemClock.uptimeMillis();
    recordUIOperation(instanceId, EVENT_LYNX_UI_UPDATE_EXTRA_DATA,
        new UpdateExtraDataPayload(sign, type, text), opTimeMillis);
  }

  public void recordInputValue(
      int instanceId, int sign, String value, int selectionStart, int selectionEnd) {
    if (value == null) {
      return;
    }
    long opTimeMillis = SystemClock.uptimeMillis();
    recordUIOperation(instanceId, EVENT_LYNX_UI_UPDATE_INPUT_VALUE,
        new UpdateInputValuePayload(sign, value, selectionStart, selectionEnd), opTimeMillis);
  }

  public void recordInitialTree(int instanceId, int[] signs, String[] tagNames, Object[] bundles,
      Object[] initialStyles, int[] parentSigns, int[] childIndexes, float[] layouts,
      boolean[] hasBounds, boolean[] hasSticky, String[] textContents) {
    Integer sessionId = mRecordingSessions.get(instanceId);
    if (sessionId == null) {
      return;
    }
    if (!isValidInitialTree(signs, tagNames, bundles, initialStyles, parentSigns, childIndexes,
            layouts, hasBounds, hasSticky)) {
      finishInitialTreeRecording(instanceId, sessionId);
      return;
    }

    int count = signs.length;
    long opTimeMillis = SystemClock.uptimeMillis();
    List<InitialNodeSnapshot> nodeSnapshots = new ArrayList<>(count);
    for (int index = 0; index < count; index++) {
      PropBundle bundle = bundles[index] instanceof PropBundle ? (PropBundle) bundles[index] : null;
      ReadableMapBuffer initialStyle = initialStyles[index] instanceof ReadableMapBuffer
          ? (ReadableMapBuffer) initialStyles[index]
          : null;
      Map<String, Object> bundleSnapshot = snapshotPropBundle(bundle);
      Map<String, Object> initialStyleSnapshot = snapshotStyleMapBuffer(initialStyle);
      int layoutOffset = index * INITIAL_TREE_LAYOUT_STRIDE;
      String textContent =
          textContents != null && textContents.length == count ? textContents[index] : null;
      nodeSnapshots.add(new InitialNodeSnapshot(signs[index], tagNames[index], bundleSnapshot,
          initialStyleSnapshot, parentSigns[index], childIndexes[index],
          layouts[layoutOffset + INITIAL_TREE_X_INDEX],
          layouts[layoutOffset + INITIAL_TREE_Y_INDEX],
          layouts[layoutOffset + INITIAL_TREE_WIDTH_INDEX],
          layouts[layoutOffset + INITIAL_TREE_HEIGHT_INDEX],
          copyFourFloats(layouts, layoutOffset + INITIAL_TREE_PADDING_INDEX),
          copyFourFloats(layouts, layoutOffset + INITIAL_TREE_MARGIN_INDEX),
          copyFourFloats(layouts, layoutOffset + INITIAL_TREE_BORDER_INDEX),
          hasBounds[index] ? copyFourFloats(layouts, layoutOffset + INITIAL_TREE_BOUNDS_INDEX)
                           : null,
          hasSticky[index] ? copyFourFloats(layouts, layoutOffset + INITIAL_TREE_STICKY_INDEX)
                           : null,
          layouts[layoutOffset + INITIAL_TREE_MAX_HEIGHT_INDEX], textContent));
    }
    final List<InitialNodeSnapshot> initialNodes = Collections.unmodifiableList(nodeSnapshots);
    mExecutor.execute(() -> {
      if (!isRecordingSession(instanceId, sessionId)) {
        return;
      }
      RecorderTreeState treeState = new RecorderTreeState(sessionId);
      List<UIOperationRecord> initialTreeOperations =
          treeState.buildInitialOperations(initialNodes, opTimeMillis);
      mTreeStates.put(instanceId, treeState);
      if (!initialTreeOperations.isEmpty()) {
        emitFrame(instanceId, TRACK_TYPE_LYNX_UI_OP, EVENT_LYNX_UI_FRAME,
            new UIFramePayload(initialTreeOperations, getScreenDensity(instanceId)), opTimeMillis);
      }
      mInitialTreePendingSessions.remove(instanceId, sessionId);
    });
  }

  public void recordTouch(
      int instanceId, int eventType, List<TouchPointPayload> points, long timestampMillis) {
    List<TouchPointPayload> pointSnapshot =
        points == null ? null : Collections.unmodifiableList(new ArrayList<>(points));
    Integer sessionId = getActiveRecordingSession(instanceId);
    if (sessionId == null) {
      return;
    }
    mExecutor.execute(() -> {
      if (!isRecordingSession(instanceId, sessionId)) {
        return;
      }
      RecorderTreeState treeState = getOrCreateTreeStateOnExecutor(instanceId, sessionId);
      emitFrame(instanceId, TRACK_TYPE_LYNX_TOUCH, eventType,
          new TouchPayload(treeState.sanitizeTouchPoints(pointSnapshot)), timestampMillis);
    });
  }

  private void recordUIOperation(int instanceId, int eventType, Object data, long opTimeMillis) {
    Integer sessionId = getActiveRecordingSession(instanceId);
    if (sessionId == null) {
      return;
    }
    mExecutor.execute(() -> {
      if (!isRecordingSession(instanceId, sessionId)
          || isInitialTreeRecordingPending(instanceId, sessionId)) {
        return;
      }
      RecorderTreeState treeState = getOrCreateTreeStateOnExecutor(instanceId, sessionId);
      UIOperationRecord operation = treeState.applyOperation(eventType, data, opTimeMillis);
      if (operation == null) {
        return;
      }
      PendingUIFrame pendingFrame = mPendingUIFrames.get(instanceId);
      if (pendingFrame == null || pendingFrame.sessionId != sessionId) {
        pendingFrame = new PendingUIFrame(sessionId);
        mPendingUIFrames.put(instanceId, pendingFrame);
      }
      pendingFrame.operations.add(operation);
      if (pendingFrame.flushScheduled) {
        return;
      }
      pendingFrame.flushScheduled = true;
      requestVSyncFlush(instanceId, sessionId, pendingFrame);
    });
  }

  public void recordFrame(int instanceId, int trackType, int eventType, Object data) {
    recordFrame(instanceId, trackType, eventType, data, SystemClock.uptimeMillis());
  }

  public void recordFrame(
      int instanceId, int trackType, int eventType, Object data, long timestampMillis) {
    Integer sessionId = getActiveRecordingSession(instanceId);
    if (sessionId == null) {
      return;
    }
    recordFrameForSession(instanceId, sessionId, trackType, eventType, data, timestampMillis);
  }

  private void recordFrameForSession(int instanceId, int sessionId, int trackType, int eventType,
      Object data, long timestampMillis) {
    final Object safeData = data == null ? new EmptyPayload() : data;
    mExecutor.execute(() -> {
      if (!isRecordingSession(instanceId, sessionId)) {
        return;
      }
      emitFrame(instanceId, trackType, eventType, safeData, timestampMillis);
    });
  }

  public JSONArray getFrames(int instanceId) {
    FrameCache frames = mFrames.get(instanceId);
    return frames == null ? new JSONArray() : frames.snapshot();
  }

  public void clearFrames(int instanceId) {
    mExecutor.execute(() -> {
      mFrames.remove(instanceId);
      clearPendingUIFrameOnExecutor(instanceId, null);
    });
  }

  private RecorderTreeState getOrCreateTreeStateOnExecutor(int instanceId, int sessionId) {
    RecorderTreeState state = mTreeStates.get(instanceId);
    if (state == null || state.sessionId != sessionId) {
      state = new RecorderTreeState(sessionId);
      mTreeStates.put(instanceId, state);
    }
    return state;
  }

  private void requestVSyncFlush(int instanceId, int sessionId, PendingUIFrame pendingFrame) {
    RequestFlushFrameCallback frameCallback = new RequestFlushFrameCallback(instanceId, sessionId);
    pendingFrame.frameCallback = frameCallback;
    UIThreadUtils.runOnUiThread(() -> Choreographer.getInstance().postFrameCallback(frameCallback));
  }

  private void flushPendingUIFrame(int instanceId, int sessionId, long frameTimeNanos) {
    PendingUIFrame pendingFrame = mPendingUIFrames.get(instanceId);
    if (pendingFrame == null || pendingFrame.sessionId != sessionId
        || !isRecordingSession(instanceId, sessionId)) {
      return;
    }
    final List<UIOperationRecord> operations;
    if (pendingFrame.operations.isEmpty()) {
      pendingFrame.operations = new ArrayList<>();
      pendingFrame.flushScheduled = false;
      pendingFrame.frameCallback = null;
      return;
    }
    operations = pendingFrame.operations;
    pendingFrame.operations = new ArrayList<>();
    pendingFrame.flushScheduled = false;
    pendingFrame.frameCallback = null;
    emitFrame(instanceId, TRACK_TYPE_LYNX_UI_OP, EVENT_LYNX_UI_FRAME,
        new UIFramePayload(operations, getScreenDensity(instanceId)),
        toUptimeMillis(frameTimeNanos));
  }

  private static long toUptimeMillis(long frameTimeNanos) {
    return SystemClock.uptimeMillis() + (frameTimeNanos - System.nanoTime()) / NANOS_PER_MILLIS;
  }

  private void emitFrame(int instanceId, int trackType, int eventType, Object data, long time) {
    try {
      JSONObject frame = new JSONObject();
      frame.put("time", time);
      frame.put("trackType", trackType);

      JSONObject event = new JSONObject();
      event.put("eventType", eventType);
      event.put("data", toJsonValue(data));
      frame.put("event", event);

      FrameCache frames = mFrames.get(instanceId);
      if (frames == null) {
        frames = new FrameCache(MAX_CACHED_FRAMES_PER_INSTANCE);
        FrameCache previous = mFrames.putIfAbsent(instanceId, frames);
        if (previous != null) {
          frames = previous;
        }
      }
      frames.add(frame);

      FrameCallback callback = mFrameCallbacks.get(instanceId);
      if (callback != null) {
        callback.onLynxFrame(frame);
      }
    } catch (JSONException ignored) {
    }
  }

  public static JSONObject toJson(PropBundle bundle) {
    return toJsonObject(snapshotPropBundle(bundle));
  }

  public static JSONObject toJson(ReadableMap map) {
    return toJsonObject(snapshotReadableMap(map));
  }

  public static JSONArray toJson(ReadableArray array) {
    return toJsonArray(snapshotReadableArray(array));
  }

  public static JSONObject toJson(ReadableMapBuffer styles) {
    return toJsonObject(snapshotStyleMapBuffer(styles));
  }

  public static JSONArray toJson(float[] values) {
    JSONArray result = new JSONArray();
    if (values == null) {
      return result;
    }
    for (float value : values) {
      try {
        result.put(value);
      } catch (JSONException ignored) {
      }
    }
    return result;
  }

  private static JSONObject toJsonObject(Map<String, Object> values) {
    Object jsonValue = toJsonValue(values == null ? Collections.emptyMap() : values);
    return jsonValue instanceof JSONObject ? (JSONObject) jsonValue : new JSONObject();
  }

  private static JSONArray toJsonArray(List<Object> values) {
    Object jsonValue = toJsonValue(values == null ? Collections.emptyList() : values);
    return jsonValue instanceof JSONArray ? (JSONArray) jsonValue : new JSONArray();
  }

  private static Map<String, Object> snapshotPropBundle(PropBundle bundle) {
    if (bundle == null) {
      return null;
    }
    Map<String, Object> result = new LinkedHashMap<>();
    result.put("props", snapshotReadableMap(bundle.getProps()));
    result.put("eventHandlers", snapshotReadableArray(bundle.getEventHandlers()));
    result.put("gestures", snapshotReadableArray(bundle.getGestures()));
    return Collections.unmodifiableMap(result);
  }

  private static Map<String, Object> snapshotStyleMapBuffer(ReadableMapBuffer styles) {
    if (styles == null) {
      return null;
    }
    return snapshotReadableMap(MapBufferUtils.convertStyleMapBufferToReadableMap(styles));
  }

  private static Map<String, Object> snapshotReadableMap(ReadableMap map) {
    if (map == null) {
      return null;
    }
    Map<String, Object> result = new LinkedHashMap<>();
    ReadableMapKeySetIterator keys = map.keySetIterator();
    while (keys.hasNextKey()) {
      String key = keys.nextKey();
      result.put(key, snapshotReadableMapValue(map, key));
    }
    return Collections.unmodifiableMap(result);
  }

  private static List<Object> snapshotReadableArray(ReadableArray array) {
    if (array == null) {
      return null;
    }
    List<Object> result = new ArrayList<>(array.size());
    for (int i = 0, size = array.size(); i < size; i++) {
      result.add(snapshotReadableArrayValue(array, i));
    }
    return Collections.unmodifiableList(result);
  }

  private static Object snapshotReadableMapValue(ReadableMap map, String key) {
    ReadableType type = map.getType(key);
    switch (type) {
      case Null:
        return null;
      case Boolean:
        return map.getBoolean(key);
      case Int:
        return map.getInt(key);
      case Long:
        return map.getLong(key);
      case Number:
        return map.getDouble(key);
      case String:
        return map.getString(key);
      case Map:
        return snapshotReadableMap(map.getMap(key));
      case Array:
        return snapshotReadableArray(map.getArray(key));
      case ByteArray:
        return copyByteArray(map.getByteArray(key));
      case ByteBuffer:
        return copyByteBuffer(map.getByteBuffer(key));
      case PiperData:
        return String.valueOf(map.getPiperData(key));
      case TemplateData:
        return String.valueOf(map.getTemplateData(key));
      case LynxObject:
      default:
        return snapshotDynamic(map.getDynamic(key));
    }
  }

  private static Object snapshotReadableArrayValue(ReadableArray array, int index) {
    ReadableType type = array.getType(index);
    switch (type) {
      case Null:
        return null;
      case Boolean:
        return array.getBoolean(index);
      case Int:
        return array.getInt(index);
      case Long:
        return array.getLong(index);
      case Number:
        return array.getDouble(index);
      case String:
        return array.getString(index);
      case Map:
        return snapshotReadableMap(array.getMap(index));
      case Array:
        return snapshotReadableArray(array.getArray(index));
      case ByteArray:
        return copyByteArray(array.getByteArray(index));
      case PiperData:
        return String.valueOf(array.getPiperData(index));
      case ByteBuffer:
      case TemplateData:
      case LynxObject:
      default:
        return snapshotDynamic(array.getDynamic(index));
    }
  }

  private static Object snapshotDynamic(Dynamic dynamic) {
    if (dynamic == null || dynamic.isNull()) {
      return null;
    }
    ReadableType type = dynamic.getType();
    switch (type) {
      case Boolean:
        return dynamic.asBoolean();
      case Int:
        return dynamic.asInt();
      case Long:
        return dynamic.asLong();
      case Number:
        return dynamic.asDouble();
      case String:
        return dynamic.asString();
      case Map:
        return snapshotReadableMap(dynamic.asMap());
      case Array:
        return snapshotReadableArray(dynamic.asArray());
      case ByteArray:
        return copyByteArray(dynamic.asByteArray());
      case Null:
        return null;
      case ByteBuffer:
      case PiperData:
      case TemplateData:
      case LynxObject:
      default:
        return String.valueOf(dynamic);
    }
  }

  private static byte[] copyByteArray(byte[] value) {
    return value == null ? null : value.clone();
  }

  private static byte[] copyByteBuffer(ByteBuffer value) {
    if (value == null) {
      return null;
    }
    ByteBuffer copySource = value.asReadOnlyBuffer();
    byte[] copy = new byte[copySource.remaining()];
    copySource.get(copy);
    return copy;
  }

  private static float[] copyFloatArray(float[] values) {
    return values == null ? null : values.clone();
  }

  private static float[] copyFourFloats(float[] values, int offset) {
    return new float[] {values[offset], values[offset + 1], values[offset + 2], values[offset + 3]};
  }

  private static boolean isValidInitialTree(int[] signs, String[] tagNames, Object[] bundles,
      Object[] initialStyles, int[] parentSigns, int[] childIndexes, float[] layouts,
      boolean[] hasBounds, boolean[] hasSticky) {
    if (signs == null || tagNames == null || bundles == null || initialStyles == null
        || parentSigns == null || childIndexes == null || layouts == null || hasBounds == null
        || hasSticky == null) {
      return false;
    }
    int count = signs.length;
    return tagNames.length == count && bundles.length == count && initialStyles.length == count
        && parentSigns.length == count && childIndexes.length == count && hasBounds.length == count
        && hasSticky.length == count && layouts.length >= count * INITIAL_TREE_LAYOUT_STRIDE;
  }

  private static Map<String, Object> sanitizeBundleForNode(
      Map<String, Object> bundle, NodeRecord node, boolean maskInputContent) {
    if (bundle == null) {
      return null;
    }
    Map<String, Object> result = new LinkedHashMap<>(bundle);
    Map<String, Object> props = getPropsFromBundle(bundle);
    if (props != null) {
      result.put("props", sanitizePropsForNode(props, node, maskInputContent));
    }
    return Collections.unmodifiableMap(result);
  }

  private static Map<String, Object> sanitizeBoxBundleForNode(
      Map<String, Object> bundle, NodeRecord node) {
    if (bundle == null) {
      return createMaskBundle(node == null ? ByteReplayMask.NONE : node.ownMask);
    }
    Map<String, Object> result = new LinkedHashMap<>(bundle);
    Map<String, Object> props = getPropsFromBundle(bundle);
    if (props != null) {
      result.put("props", sanitizeBoxPropsForNode(props, node));
    }
    return Collections.unmodifiableMap(result);
  }

  private static Map<String, Object> sanitizePropsForNode(
      Map<String, Object> props, NodeRecord node, boolean maskInputContent) {
    Map<String, Object> result = new LinkedHashMap<>();
    for (Map.Entry<String, Object> entry : props.entrySet()) {
      String key = entry.getKey();
      Object value = entry.getValue();
      if ((maskInputContent || (node != null && node.isPasswordInput)) && isInputContentKey(key)) {
        result.put(key, maskInputContentValue(value));
        continue;
      }
      result.put(key, value);
    }
    return Collections.unmodifiableMap(result);
  }

  private static Map<String, Object> sanitizeBoxPropsForNode(
      Map<String, Object> props, NodeRecord node) {
    Map<String, Object> result = new LinkedHashMap<>();
    for (Map.Entry<String, Object> entry : props.entrySet()) {
      String key = entry.getKey();
      Object value = entry.getValue();
      if (isBoxSensitivePropKey(key)) {
        result.put(key, maskInputContentValue(value));
        continue;
      }
      result.put(key, value);
    }
    String maskValue = getMaskValue(node == null ? ByteReplayMask.NONE : node.ownMask);
    if (maskValue != null && !result.containsKey(LYNX_BYTE_REPLAY_MASK_PROP)) {
      result.put(LYNX_BYTE_REPLAY_MASK_PROP, maskValue);
    }
    return Collections.unmodifiableMap(result);
  }

  private static Map<String, Object> sanitizeInputContentMap(Map<String, Object> values) {
    if (values == null) {
      return null;
    }
    Map<String, Object> result = new LinkedHashMap<>();
    for (Map.Entry<String, Object> entry : values.entrySet()) {
      String key = entry.getKey();
      Object value = entry.getValue();
      if (isInputContentKey(key)) {
        result.put(key, maskInputContentValue(value));
      } else {
        result.put(key, sanitizeInputContentValue(value));
      }
    }
    return Collections.unmodifiableMap(result);
  }

  private static Object maskInputContentValue(Object value) {
    if (value instanceof CharSequence) {
      return maskText(String.valueOf(value));
    }
    return MASKED_TEXT;
  }

  private static String maskText(String text) {
    if (text == null) {
      return null;
    }
    int length = text.codePointCount(0, text.length());
    StringBuilder result = new StringBuilder(length);
    for (int index = 0; index < length; ++index) {
      result.append(MASKED_TEXT);
    }
    return result.toString();
  }

  private static Map<String, Object> createMaskBundle(ByteReplayMask mask) {
    String maskValue = getMaskValue(mask);
    if (maskValue == null) {
      return null;
    }
    Map<String, Object> props = new LinkedHashMap<>();
    props.put(LYNX_BYTE_REPLAY_MASK_PROP, maskValue);
    Map<String, Object> bundle = new LinkedHashMap<>();
    bundle.put("props", Collections.unmodifiableMap(props));
    return Collections.unmodifiableMap(bundle);
  }

  private static String getMaskValue(ByteReplayMask mask) {
    switch (mask) {
      case TRUE:
        return MASK_VALUE_TRUE;
      case IGNORE:
        return MASK_VALUE_IGNORE;
      case NAME:
        return MASK_VALUE_NAME;
      case STRICT:
        return MASK_VALUE_STRICT;
      case NONE:
      default:
        return null;
    }
  }

  private static Object sanitizeInputContentValue(Object value) {
    Map<String, Object> map = toStringObjectMap(value);
    if (map != null) {
      return sanitizeInputContentMap(map);
    }
    if (value instanceof Iterable) {
      List<Object> result = new ArrayList<>();
      for (Object item : (Iterable<?>) value) {
        result.add(sanitizeInputContentValue(item));
      }
      return Collections.unmodifiableList(result);
    }
    if (value != null && value.getClass().isArray()) {
      List<Object> result = new ArrayList<>();
      int length = Array.getLength(value);
      for (int i = 0; i < length; i++) {
        result.add(sanitizeInputContentValue(Array.get(value, i)));
      }
      return Collections.unmodifiableList(result);
    }
    return value;
  }

  private static Map<String, Object> getPropsFromBundle(Map<String, Object> bundle) {
    return bundle == null ? null : toStringObjectMap(bundle.get("props"));
  }

  private static boolean hasMaskKey(Map<String, Object> bundle) {
    Map<String, Object> props = getPropsFromBundle(bundle);
    return props != null && props.containsKey(LYNX_BYTE_REPLAY_MASK_PROP);
  }

  private static ByteReplayMask parseMask(Map<String, Object> bundle) {
    Map<String, Object> props = getPropsFromBundle(bundle);
    if (props == null) {
      return ByteReplayMask.NONE;
    }
    Object value = props.get(LYNX_BYTE_REPLAY_MASK_PROP);
    return parseMaskValue(value);
  }

  private static ByteReplayMask parseMaskValue(Object value) {
    if (value == null) {
      return ByteReplayMask.NONE;
    }
    String mask = String.valueOf(value);
    if (MASK_VALUE_TRUE.equals(mask)) {
      return ByteReplayMask.TRUE;
    }
    if (MASK_VALUE_IGNORE.equals(mask)) {
      return ByteReplayMask.IGNORE;
    }
    if (MASK_VALUE_NAME.equals(mask)) {
      return ByteReplayMask.NAME;
    }
    if (MASK_VALUE_STRICT.equals(mask)) {
      return ByteReplayMask.STRICT;
    }
    return ByteReplayMask.NONE;
  }

  private static boolean hasInputTypeKey(Map<String, Object> bundle) {
    Map<String, Object> props = getPropsFromBundle(bundle);
    return props != null && props.containsKey(INPUT_TYPE_PROP);
  }

  private static boolean isPasswordInputType(Map<String, Object> bundle) {
    Map<String, Object> props = getPropsFromBundle(bundle);
    if (props == null) {
      return false;
    }
    Object value = props.get(INPUT_TYPE_PROP);
    return INPUT_PASSWORD_TYPE.equals(value);
  }

  private static boolean isInputTag(String tagName) {
    return INPUT_TAG.equals(tagName) || X_INPUT_TAG.equals(tagName);
  }

  private static boolean isTextInputTag(String tagName) {
    return isInputTag(tagName) || TEXTAREA_TAG.equals(tagName) || X_TEXTAREA_TAG.equals(tagName);
  }

  private static boolean isInputContentKey(String key) {
    return "value".equals(key) || "text".equals(key) || "default-value".equals(key)
        || "defaultValue".equals(key);
  }

  private static boolean isBoxSensitivePropKey(String key) {
    return isInputContentKey(key) || isMediaSourceKey(key);
  }

  private static boolean isMediaSourceKey(String key) {
    return "src".equals(key) || "source".equals(key) || "url".equals(key) || "uri".equals(key)
        || "poster".equals(key);
  }

  private static boolean isMediaTag(String tagName) {
    return "image".equals(tagName) || "inline-image".equals(tagName) || "video".equals(tagName)
        || "audio".equals(tagName) || "x-video-engine".equals(tagName);
  }

  @SuppressWarnings("unchecked")
  private static Map<String, Object> toStringObjectMap(Object value) {
    if (!(value instanceof Map)) {
      return null;
    }
    Map<?, ?> source = (Map<?, ?>) value;
    Map<String, Object> result = new LinkedHashMap<>();
    for (Map.Entry<?, ?> entry : source.entrySet()) {
      Object key = entry.getKey();
      if (key != null) {
        result.put(String.valueOf(key), entry.getValue());
      }
    }
    return result;
  }

  public static void put(JSONObject target, String key, Object value) {
    try {
      target.put(key, toJsonValue(value));
    } catch (JSONException ignored) {
    }
  }

  private static Object toJsonValue(Object value) {
    if (value == null) {
      return JSONObject.NULL;
    }
    if (value instanceof JSONObject || value instanceof JSONArray || value == JSONObject.NULL) {
      return value;
    }
    if (value instanceof EmptyPayload) {
      return new JSONObject();
    }
    if (value instanceof UIOperationRecord) {
      UIOperationRecord record = (UIOperationRecord) value;
      JSONObject result = new JSONObject();
      put(result, "eventType", record.eventType);
      put(result, "opTime", record.opTime);
      put(result, "data", record.payload);
      return result;
    }
    if (value instanceof UIFramePayload) {
      UIFramePayload payload = (UIFramePayload) value;
      JSONObject result = new JSONObject();
      put(result, "operations", payload.operations);
      if (payload.screenDensity != null) {
        put(result, "screenDensity", payload.screenDensity);
      }
      return result;
    }
    if (value instanceof CreateNodePayload) {
      CreateNodePayload payload = (CreateNodePayload) value;
      JSONObject result = new JSONObject();
      put(result, "sign", payload.sign);
      put(result, "tagName", payload.tagName);
      put(result, "bundle", payload.bundle);
      put(result, "initialStyles", payload.initialStyles);
      return result;
    }
    if (value instanceof NodeRelationPayload) {
      NodeRelationPayload payload = (NodeRelationPayload) value;
      JSONObject result = new JSONObject();
      put(result, "parentSign", payload.parentSign);
      put(result, "childSign", payload.childSign);
      put(result, "index", payload.index);
      return result;
    }
    if (value instanceof UpdatePropsPayload) {
      UpdatePropsPayload payload = (UpdatePropsPayload) value;
      JSONObject result = new JSONObject();
      put(result, "sign", payload.sign);
      if (payload.tendToFlatten != null) {
        put(result, "tendToFlatten", payload.tendToFlatten);
      }
      put(result, "bundle", payload.bundle);
      put(result, "styles", payload.styles);
      return result;
    }
    if (value instanceof UpdateLayoutPayload) {
      UpdateLayoutPayload payload = (UpdateLayoutPayload) value;
      JSONObject result = new JSONObject();
      put(result, "sign", payload.sign);
      put(result, "x", payload.x);
      put(result, "y", payload.y);
      put(result, "width", payload.width);
      put(result, "height", payload.height);
      put(result, "paddings", payload.paddings);
      put(result, "margins", payload.margins);
      put(result, "borders", payload.borders);
      put(result, "bounds", payload.bounds);
      put(result, "sticky", payload.sticky);
      put(result, "maxHeight", payload.maxHeight);
      return result;
    }
    if (value instanceof UpdateExtraDataPayload) {
      UpdateExtraDataPayload payload = (UpdateExtraDataPayload) value;
      JSONObject result = new JSONObject();
      put(result, "sign", payload.sign);
      put(result, "type", payload.type);
      put(result, "text", payload.text);
      return result;
    }
    if (value instanceof UpdateInputValuePayload) {
      UpdateInputValuePayload payload = (UpdateInputValuePayload) value;
      JSONObject result = new JSONObject();
      put(result, "sign", payload.sign);
      put(result, "value", payload.value);
      put(result, "selectionStart", payload.selectionStart);
      put(result, "selectionEnd", payload.selectionEnd);
      return result;
    }
    if (value instanceof InvokePayload) {
      InvokePayload payload = (InvokePayload) value;
      JSONObject result = new JSONObject();
      put(result, "sign", payload.sign);
      put(result, "method", payload.method);
      put(result, "params", payload.params);
      return result;
    }
    if (value instanceof TouchPayload) {
      TouchPayload payload = (TouchPayload) value;
      JSONObject result = new JSONObject();
      put(result, "points", payload.points);
      return result;
    }
    if (value instanceof TouchPointPayload) {
      TouchPointPayload payload = (TouchPointPayload) value;
      JSONObject result = new JSONObject();
      put(result, "x", payload.x);
      put(result, "y", payload.y);
      put(result, "touchId", payload.touchId);
      put(result, "targetId", payload.targetId);
      return result;
    }
    if (value instanceof PropBundle) {
      return toJson((PropBundle) value);
    }
    if (value instanceof ReadableMap) {
      return toJson((ReadableMap) value);
    }
    if (value instanceof ReadableArray) {
      return toJson((ReadableArray) value);
    }
    if (value instanceof ReadableMapBuffer) {
      return toJson((ReadableMapBuffer) value);
    }
    if (value instanceof float[]) {
      return toJson((float[]) value);
    }
    if (value instanceof Map) {
      JSONObject result = new JSONObject();
      @SuppressWarnings("unchecked") Map<Object, Object> map = (Map<Object, Object>) value;
      for (Map.Entry<Object, Object> entry : map.entrySet()) {
        try {
          result.put(String.valueOf(entry.getKey()), toJsonValue(entry.getValue()));
        } catch (JSONException ignored) {
        }
      }
      return result;
    }
    if (value instanceof Iterable) {
      JSONArray result = new JSONArray();
      for (Object item : (Iterable<?>) value) {
        result.put(toJsonValue(item));
      }
      return result;
    }
    if (value.getClass().isArray()) {
      JSONArray result = new JSONArray();
      int length = Array.getLength(value);
      for (int i = 0; i < length; i++) {
        result.put(toJsonValue(Array.get(value, i)));
      }
      return result;
    }
    if (value instanceof CharSequence || value instanceof Boolean || value instanceof Number) {
      return value;
    }
    if (value instanceof Enum<?>) {
      return ((Enum<?>) value).name();
    }
    return String.valueOf(value);
  }

  private enum ByteReplayMask { NONE, TRUE, IGNORE, NAME, STRICT }

  private static final class RecorderTreeState {
    private final int sessionId;
    private final Map<Integer, NodeRecord> nodes = new HashMap<>();

    private RecorderTreeState(int sessionId) {
      this.sessionId = sessionId;
    }

    private List<UIOperationRecord> buildInitialOperations(
        List<InitialNodeSnapshot> initialNodes, long opTime) {
      nodes.clear();
      for (InitialNodeSnapshot node : initialNodes) {
        NodeRecord record = getOrCreateNode(node.sign);
        record.tagName = node.tagName;
        updateNodeFromBundle(record, node.bundle, true);
      }
      for (InitialNodeSnapshot node : initialNodes) {
        if (node.parentSign >= 0) {
          setParent(node.sign, node.parentSign);
        }
      }

      List<UIOperationRecord> operations = new ArrayList<>(initialNodes.size() * 4);
      for (InitialNodeSnapshot node : initialNodes) {
        if (shouldDropNode(node.sign)) {
          continue;
        }
        boolean boxNode = isBoxNode(node.sign);
        operations.add(new UIOperationRecord(EVENT_LYNX_UI_CREATE_NODE,
            new CreateNodePayload(node.sign, boxNode ? VIEW_TAG : node.tagName,
                boxNode ? getBoxNodeBundle(node.sign, node.bundle)
                        : sanitizeBundleForNode(
                            node.bundle, nodes.get(node.sign), shouldMaskText(node.sign)),
                node.initialStyles),
            opTime));
        Integer parentSign = getRecordedParentSign(node.sign);
        if (parentSign != null) {
          operations.add(new UIOperationRecord(EVENT_LYNX_UI_INSERT_NODE,
              new NodeRelationPayload(parentSign, node.sign, node.childIndex), opTime));
        }
        if (!boxNode) {
          operations.add(new UIOperationRecord(EVENT_LYNX_UI_UPDATE_PROPS,
              new UpdatePropsPayload(node.sign, null,
                  sanitizeBundleForNode(
                      node.bundle, nodes.get(node.sign), shouldMaskText(node.sign)),
                  node.initialStyles),
              opTime));
        }
        operations.add(new UIOperationRecord(EVENT_LYNX_UI_UPDATE_LAYOUT,
            new UpdateLayoutPayload(node.sign, node.x, node.y, node.width, node.height,
                copyFloatArray(node.paddings), copyFloatArray(node.margins),
                copyFloatArray(node.borders), copyFloatArray(node.bounds),
                copyFloatArray(node.sticky), node.maxHeight),
            opTime));
        if (!boxNode && node.text != null) {
          operations.add(new UIOperationRecord(EVENT_LYNX_UI_UPDATE_EXTRA_DATA,
              new UpdateExtraDataPayload(node.sign, TEXT_EXTRA_DATA_TYPE,
                  shouldMaskText(node.sign) ? maskText(node.text) : node.text),
              opTime));
        }
      }
      return Collections.unmodifiableList(operations);
    }

    private UIOperationRecord applyOperation(int eventType, Object payload, long opTime) {
      switch (eventType) {
        case EVENT_LYNX_UI_CREATE_NODE:
          return applyCreateNode((CreateNodePayload) payload, opTime);
        case EVENT_LYNX_UI_INSERT_NODE:
          return applyInsertNode((NodeRelationPayload) payload, opTime);
        case EVENT_LYNX_UI_REMOVE_NODE:
          return applyRemoveNode((NodeRelationPayload) payload, opTime);
        case EVENT_LYNX_UI_DESTROY_NODE:
          return applyDestroyNode((NodeRelationPayload) payload, opTime);
        case EVENT_LYNX_UI_UPDATE_PROPS:
          return applyUpdateProps((UpdatePropsPayload) payload, opTime);
        case EVENT_LYNX_UI_UPDATE_LAYOUT:
          return applyUpdateLayout((UpdateLayoutPayload) payload, opTime);
        case EVENT_LYNX_UI_INVOKE:
          return applyInvoke((InvokePayload) payload, opTime);
        case EVENT_LYNX_UI_UPDATE_EXTRA_DATA:
          return applyUpdateExtraData((UpdateExtraDataPayload) payload, opTime);
        case EVENT_LYNX_UI_UPDATE_INPUT_VALUE:
          return applyUpdateInputValue((UpdateInputValuePayload) payload, opTime);
        default:
          return new UIOperationRecord(eventType, payload, opTime);
      }
    }

    private List<TouchPointPayload> sanitizeTouchPoints(List<TouchPointPayload> points) {
      if (points == null) {
        return null;
      }
      List<TouchPointPayload> result = new ArrayList<>(points.size());
      for (TouchPointPayload point : points) {
        Integer targetId = point.targetId == null ? null : getVisibleTargetSign(point.targetId);
        result.add(new TouchPointPayload(point.x, point.y, point.touchId, targetId));
      }
      return Collections.unmodifiableList(result);
    }

    private UIOperationRecord applyCreateNode(CreateNodePayload payload, long opTime) {
      NodeRecord node = getOrCreateNode(payload.sign);
      node.tagName = payload.tagName;
      updateNodeFromBundle(node, payload.bundle, true);
      if (shouldDropNode(payload.sign)) {
        return null;
      }
      if (isBoxNode(payload.sign)) {
        return new UIOperationRecord(EVENT_LYNX_UI_CREATE_NODE,
            new CreateNodePayload(payload.sign, VIEW_TAG,
                getBoxNodeBundle(payload.sign, payload.bundle), payload.initialStyles),
            opTime);
      }
      return new UIOperationRecord(EVENT_LYNX_UI_CREATE_NODE,
          new CreateNodePayload(payload.sign, payload.tagName,
              sanitizeBundleForNode(payload.bundle, node, shouldMaskText(payload.sign)),
              payload.initialStyles),
          opTime);
    }

    private UIOperationRecord applyInsertNode(NodeRelationPayload payload, long opTime) {
      setParent(payload.childSign, payload.parentSign);
      if (shouldDropNode(payload.childSign)) {
        return null;
      }
      Integer parentSign = getRecordedParentSign(payload.childSign);
      if (parentSign == null) {
        return null;
      }
      return new UIOperationRecord(EVENT_LYNX_UI_INSERT_NODE,
          new NodeRelationPayload(parentSign, payload.childSign, payload.index), opTime);
    }

    private UIOperationRecord applyRemoveNode(NodeRelationPayload payload, long opTime) {
      UIOperationRecord operation = null;
      if (!shouldDropNode(payload.childSign)) {
        Integer parentSign = getRecordedParentSign(payload.childSign);
        if (parentSign != null) {
          operation = new UIOperationRecord(EVENT_LYNX_UI_REMOVE_NODE,
              new NodeRelationPayload(parentSign, payload.childSign, null), opTime);
        }
      }
      detachFromParent(payload.childSign);
      return operation;
    }

    private UIOperationRecord applyDestroyNode(NodeRelationPayload payload, long opTime) {
      UIOperationRecord operation = null;
      if (!shouldDropNode(payload.childSign)) {
        Integer parentSign = getRecordedParentSign(payload.childSign);
        operation = new UIOperationRecord(EVENT_LYNX_UI_DESTROY_NODE,
            new NodeRelationPayload(
                parentSign == null ? payload.parentSign : parentSign, payload.childSign, null),
            opTime);
      }
      destroySubtree(payload.childSign);
      return operation;
    }

    private UIOperationRecord applyUpdateProps(UpdatePropsPayload payload, long opTime) {
      NodeRecord node = getOrCreateNode(payload.sign);
      updateNodeFromBundle(node, payload.bundle, false);
      if (shouldDropNode(payload.sign)) {
        return null;
      }
      boolean boxNode = isBoxNode(payload.sign);
      return new UIOperationRecord(EVENT_LYNX_UI_UPDATE_PROPS,
          new UpdatePropsPayload(payload.sign, payload.tendToFlatten,
              boxNode ? getBoxNodeBundle(payload.sign, payload.bundle)
                      : sanitizeBundleForNode(payload.bundle, node, shouldMaskText(payload.sign)),
              payload.styles),
          opTime);
    }

    private UIOperationRecord applyUpdateLayout(UpdateLayoutPayload payload, long opTime) {
      if (shouldDropNode(payload.sign)) {
        return null;
      }
      return new UIOperationRecord(EVENT_LYNX_UI_UPDATE_LAYOUT, payload, opTime);
    }

    private UIOperationRecord applyInvoke(InvokePayload payload, long opTime) {
      if (shouldDropNode(payload.sign) || isBoxNode(payload.sign)) {
        return null;
      }
      NodeRecord node = nodes.get(payload.sign);
      Map<String, Object> params =
          (node != null && node.isPasswordInput) || shouldMaskText(payload.sign)
          ? sanitizeInputContentMap(payload.params)
          : payload.params;
      return new UIOperationRecord(
          EVENT_LYNX_UI_INVOKE, new InvokePayload(payload.sign, payload.method, params), opTime);
    }

    private UIOperationRecord applyUpdateExtraData(UpdateExtraDataPayload payload, long opTime) {
      if (shouldDropNode(payload.sign) || isBoxNode(payload.sign)) {
        return null;
      }
      String text = payload.text;
      if (TEXT_EXTRA_DATA_TYPE.equals(payload.type) && shouldMaskText(payload.sign)) {
        text = maskText(payload.text);
      }
      return new UIOperationRecord(EVENT_LYNX_UI_UPDATE_EXTRA_DATA,
          new UpdateExtraDataPayload(payload.sign, payload.type, text), opTime);
    }

    private UIOperationRecord applyUpdateInputValue(UpdateInputValuePayload payload, long opTime) {
      if (shouldDropNode(payload.sign) || isBoxNode(payload.sign)) {
        return null;
      }
      NodeRecord node = nodes.get(payload.sign);
      if (node == null || !isTextInputTag(node.tagName)) {
        return null;
      }
      String value = payload.value;
      if (node.isPasswordInput || shouldMaskText(payload.sign)) {
        value = maskText(payload.value);
      }
      return new UIOperationRecord(EVENT_LYNX_UI_UPDATE_INPUT_VALUE,
          new UpdateInputValuePayload(
              payload.sign, value, payload.selectionStart, payload.selectionEnd),
          opTime);
    }

    private NodeRecord getOrCreateNode(int sign) {
      NodeRecord node = nodes.get(sign);
      if (node == null) {
        node = new NodeRecord(sign);
        nodes.put(sign, node);
      }
      return node;
    }

    private Map<String, Object> getBoxNodeBundle(int sign, Map<String, Object> bundle) {
      NodeRecord node = nodes.get(sign);
      return sanitizeBoxBundleForNode(bundle, node);
    }

    private void updateNodeFromBundle(
        NodeRecord node, Map<String, Object> bundle, boolean resetMissingMask) {
      if (resetMissingMask || hasMaskKey(bundle)) {
        node.ownMask = parseMask(bundle);
      }
      if (isInputTag(node.tagName)) {
        if (resetMissingMask || hasInputTypeKey(bundle)) {
          node.isPasswordInput = isPasswordInputType(bundle);
        }
      } else {
        node.isPasswordInput = false;
      }
    }

    private void setParent(int childSign, int parentSign) {
      NodeRecord child = getOrCreateNode(childSign);
      detachFromParent(childSign);
      child.parentSign = parentSign;
      if (parentSign >= 0) {
        NodeRecord parent = getOrCreateNode(parentSign);
        if (!parent.children.contains(childSign)) {
          parent.children.add(childSign);
        }
      }
    }

    private void detachFromParent(int sign) {
      NodeRecord child = nodes.get(sign);
      if (child == null || child.parentSign < 0) {
        return;
      }
      NodeRecord parent = nodes.get(child.parentSign);
      if (parent != null) {
        parent.children.remove(Integer.valueOf(sign));
      }
      child.parentSign = -1;
    }

    private void destroySubtree(int sign) {
      NodeRecord node = nodes.get(sign);
      if (node == null) {
        return;
      }
      List<Integer> children = new ArrayList<>(node.children);
      for (Integer childSign : children) {
        destroySubtree(childSign);
      }
      detachFromParent(sign);
      nodes.remove(sign);
    }

    private boolean shouldDropNode(int sign) {
      return isHiddenByBoxAncestor(sign);
    }

    private boolean isBoxNode(int sign) {
      return isIgnoredRoot(sign) || isFilteredMedia(sign);
    }

    private boolean isIgnoredRoot(int sign) {
      NodeRecord node = nodes.get(sign);
      return node != null && node.ownMask == ByteReplayMask.IGNORE && !isHiddenByBoxAncestor(sign);
    }

    private boolean isFilteredMedia(int sign) {
      NodeRecord node = nodes.get(sign);
      return node != null && isMediaTag(node.tagName) && hasStrictMaskOnPath(sign)
          && !isHiddenByBoxAncestor(sign);
    }

    private boolean shouldMaskText(int sign) {
      NodeRecord node = nodes.get(sign);
      if (node != null && node.isPasswordInput) {
        return true;
      }
      if (node != null && node.ownMask == ByteReplayMask.NAME) {
        return true;
      }
      return hasSubtreeTextMaskOnPath(sign);
    }

    private boolean hasSubtreeTextMaskOnPath(int sign) {
      Integer current = sign;
      int guard = nodes.size() + 1;
      while (current != null && guard-- > 0) {
        NodeRecord node = nodes.get(current);
        if (node == null) {
          return false;
        }
        if (node.ownMask == ByteReplayMask.TRUE || node.ownMask == ByteReplayMask.STRICT) {
          return true;
        }
        current = node.parentSign >= 0 ? node.parentSign : null;
      }
      return false;
    }

    private boolean hasStrictMaskOnPath(int sign) {
      Integer current = sign;
      int guard = nodes.size() + 1;
      while (current != null && guard-- > 0) {
        NodeRecord node = nodes.get(current);
        if (node == null) {
          return false;
        }
        if (node.ownMask == ByteReplayMask.STRICT) {
          return true;
        }
        current = node.parentSign >= 0 ? node.parentSign : null;
      }
      return false;
    }

    private boolean isHiddenByBoxAncestor(int sign) {
      NodeRecord node = nodes.get(sign);
      if (node == null) {
        return false;
      }
      Integer current = node.parentSign >= 0 ? node.parentSign : null;
      int guard = nodes.size() + 1;
      while (current != null && guard-- > 0) {
        if (isBoxNode(current)) {
          return true;
        }
        NodeRecord parent = nodes.get(current);
        if (parent == null) {
          return false;
        }
        current = parent.parentSign >= 0 ? parent.parentSign : null;
      }
      return false;
    }

    private Integer getRecordedParentSign(int sign) {
      NodeRecord node = nodes.get(sign);
      if (node == null || node.parentSign < 0 || shouldDropNode(node.parentSign)) {
        return null;
      }
      return node.parentSign;
    }

    private Integer getVisibleTargetSign(int sign) {
      NodeRecord node = nodes.get(sign);
      if (node == null || !shouldDropNode(sign)) {
        return sign;
      }
      Integer current = sign;
      int guard = nodes.size() + 1;
      while (current != null && guard-- > 0) {
        if (isBoxNode(current)) {
          return current;
        }
        NodeRecord currentNode = nodes.get(current);
        if (currentNode == null) {
          return null;
        }
        current = currentNode.parentSign >= 0 ? currentNode.parentSign : null;
      }
      return null;
    }
  }

  private static final class NodeRecord {
    private final int sign;
    private String tagName;
    private int parentSign = -1;
    private final List<Integer> children = new ArrayList<>();
    private ByteReplayMask ownMask = ByteReplayMask.NONE;
    private boolean isPasswordInput = false;

    private NodeRecord(int sign) {
      this.sign = sign;
    }
  }

  private static final class RecorderThreadFactory implements ThreadFactory {
    private final AtomicInteger mThreadNumber = new AtomicInteger(1);

    @Override
    public Thread newThread(Runnable r) {
      Thread thread = new Thread(r, "lynx-frame-recorder-" + mThreadNumber.getAndIncrement());
      thread.setPriority(Thread.NORM_PRIORITY - 1);
      return thread;
    }
  }

  private static final class PendingUIFrame {
    private final int sessionId;
    private List<UIOperationRecord> operations = new ArrayList<>();
    private boolean flushScheduled = false;
    private Choreographer.FrameCallback frameCallback = null;

    private PendingUIFrame(int sessionId) {
      this.sessionId = sessionId;
    }
  }

  public boolean isRecordingEnabled(int instanceId) {
    return instanceId >= 0 && getActiveRecordingSession(instanceId) != null;
  }

  private Integer getActiveRecordingSession(int instanceId) {
    Integer sessionId = mRecordingSessions.get(instanceId);
    if (sessionId == null) {
      return null;
    }
    Integer stoppingSessionId = mStoppingSessions.get(instanceId);
    if (stoppingSessionId != null && stoppingSessionId.equals(sessionId)) {
      return null;
    }
    return sessionId;
  }

  private boolean isRecordingSession(int instanceId, int sessionId) {
    Integer currentSessionId = mRecordingSessions.get(instanceId);
    return currentSessionId != null && currentSessionId == sessionId;
  }

  private boolean isInitialTreeRecordingPending(int instanceId, int sessionId) {
    Integer pendingSessionId = mInitialTreePendingSessions.get(instanceId);
    return pendingSessionId != null && pendingSessionId == sessionId;
  }

  private void finishInitialTreeRecording(int instanceId, int sessionId) {
    mExecutor.execute(() -> {
      if (isRecordingSession(instanceId, sessionId)) {
        mInitialTreePendingSessions.remove(instanceId, sessionId);
      }
    });
  }

  private void clearPendingUIFrameOnExecutor(int instanceId, Integer sessionId) {
    PendingUIFrame pendingFrame = mPendingUIFrames.get(instanceId);
    if (pendingFrame == null) {
      return;
    }
    if (sessionId != null && pendingFrame.sessionId != sessionId) {
      return;
    }
    mPendingUIFrames.remove(instanceId);
    Choreographer.FrameCallback frameCallback = pendingFrame.frameCallback;
    pendingFrame.operations = new ArrayList<>();
    pendingFrame.flushScheduled = false;
    pendingFrame.frameCallback = null;
    if (frameCallback != null) {
      UIThreadUtils.runOnUiThread(
          () -> Choreographer.getInstance().removeFrameCallback(frameCallback));
    }
  }

  private void flushPendingUIFrameOnExecutor(int instanceId, int sessionId, long timestampMillis) {
    PendingUIFrame pendingFrame = mPendingUIFrames.get(instanceId);
    if (pendingFrame == null || pendingFrame.sessionId != sessionId) {
      return;
    }
    mPendingUIFrames.remove(instanceId);
    Choreographer.FrameCallback frameCallback = pendingFrame.frameCallback;
    List<UIOperationRecord> operations = pendingFrame.operations;
    pendingFrame.operations = new ArrayList<>();
    pendingFrame.flushScheduled = false;
    pendingFrame.frameCallback = null;
    if (frameCallback != null) {
      UIThreadUtils.runOnUiThread(
          () -> Choreographer.getInstance().removeFrameCallback(frameCallback));
    }
    if (!operations.isEmpty()) {
      emitFrame(instanceId, TRACK_TYPE_LYNX_UI_OP, EVENT_LYNX_UI_FRAME,
          new UIFramePayload(operations, getScreenDensity(instanceId)), timestampMillis);
    }
  }

  private Float getScreenDensity(int instanceId) {
    Float screenDensity = mScreenDensities.get(instanceId);
    return screenDensity != null && screenDensity > 0.f ? screenDensity : null;
  }

  private static final class FrameCache {
    private final int mCapacity;
    private final Deque<JSONObject> mFrames = new ArrayDeque<>();

    private FrameCache(int capacity) {
      mCapacity = capacity;
    }

    private synchronized void add(JSONObject frame) {
      if (mCapacity <= 0) {
        return;
      }
      if (mFrames.size() >= mCapacity) {
        mFrames.removeFirst();
      }
      mFrames.addLast(frame);
    }

    private synchronized JSONArray snapshot() {
      JSONArray result = new JSONArray();
      for (JSONObject frame : mFrames) {
        result.put(frame);
      }
      return result;
    }
  }

  private final class RequestFlushFrameCallback implements Choreographer.FrameCallback {
    private final int mInstanceId;
    private final int mSessionId;

    private RequestFlushFrameCallback(int instanceId, int sessionId) {
      mInstanceId = instanceId;
      mSessionId = sessionId;
    }

    @Override
    public void doFrame(long frameTimeNanos) {
      mExecutor.execute(() -> flushPendingUIFrame(mInstanceId, mSessionId, frameTimeNanos));
    }
  }

  public static final class TouchPointPayload {
    public final float x;
    public final float y;
    public final int touchId;
    public final Integer targetId;

    public TouchPointPayload(float x, float y, int touchId, Integer targetId) {
      this.x = x;
      this.y = y;
      this.touchId = touchId;
      this.targetId = targetId;
    }
  }

  private static final class EmptyPayload {}

  private static final class UIOperationRecord {
    private final int eventType;
    private final Object payload;
    private final long opTime;

    private UIOperationRecord(int eventType, Object payload, long opTime) {
      this.eventType = eventType;
      this.payload = payload;
      this.opTime = opTime;
    }
  }

  private static final class UIFramePayload {
    private final List<UIOperationRecord> operations;
    private final Float screenDensity;

    private UIFramePayload(List<UIOperationRecord> operations, Float screenDensity) {
      this.operations = operations;
      this.screenDensity = screenDensity;
    }
  }

  private static final class InitialNodeSnapshot {
    private final int sign;
    private final String tagName;
    private final Map<String, Object> bundle;
    private final Map<String, Object> initialStyles;
    private final int parentSign;
    private final int childIndex;
    private final float x;
    private final float y;
    private final float width;
    private final float height;
    private final float[] paddings;
    private final float[] margins;
    private final float[] borders;
    private final float[] bounds;
    private final float[] sticky;
    private final float maxHeight;
    private final String text;

    private InitialNodeSnapshot(int sign, String tagName, Map<String, Object> bundle,
        Map<String, Object> initialStyles, int parentSign, int childIndex, float x, float y,
        float width, float height, float[] paddings, float[] margins, float[] borders,
        float[] bounds, float[] sticky, float maxHeight, String text) {
      this.sign = sign;
      this.tagName = tagName;
      this.bundle = bundle;
      this.initialStyles = initialStyles;
      this.parentSign = parentSign;
      this.childIndex = childIndex;
      this.x = x;
      this.y = y;
      this.width = width;
      this.height = height;
      this.paddings = paddings;
      this.margins = margins;
      this.borders = borders;
      this.bounds = bounds;
      this.sticky = sticky;
      this.maxHeight = maxHeight;
      this.text = text;
    }
  }

  private static final class CreateNodePayload {
    private final int sign;
    private final String tagName;
    private final Map<String, Object> bundle;
    private final Map<String, Object> initialStyles;

    private CreateNodePayload(
        int sign, String tagName, Map<String, Object> bundle, Map<String, Object> initialStyles) {
      this.sign = sign;
      this.tagName = tagName;
      this.bundle = bundle;
      this.initialStyles = initialStyles;
    }
  }

  private static final class NodeRelationPayload {
    private final int parentSign;
    private final int childSign;
    private final Integer index;

    private NodeRelationPayload(int parentSign, int childSign, Integer index) {
      this.parentSign = parentSign;
      this.childSign = childSign;
      this.index = index;
    }
  }

  private static final class UpdatePropsPayload {
    private final int sign;
    private final Boolean tendToFlatten;
    private final Map<String, Object> bundle;
    private final Map<String, Object> styles;

    private UpdatePropsPayload(
        int sign, Boolean tendToFlatten, Map<String, Object> bundle, Map<String, Object> styles) {
      this.sign = sign;
      this.tendToFlatten = tendToFlatten;
      this.bundle = bundle;
      this.styles = styles;
    }
  }

  private static final class UpdateLayoutPayload {
    private final int sign;
    private final float x;
    private final float y;
    private final float width;
    private final float height;
    private final float[] paddings;
    private final float[] margins;
    private final float[] borders;
    private final float[] bounds;
    private final float[] sticky;
    private final float maxHeight;

    private UpdateLayoutPayload(int sign, float x, float y, float width, float height,
        float[] paddings, float[] margins, float[] borders, float[] bounds, float[] sticky,
        float maxHeight) {
      this.sign = sign;
      this.x = x;
      this.y = y;
      this.width = width;
      this.height = height;
      this.paddings = paddings;
      this.margins = margins;
      this.borders = borders;
      this.bounds = bounds;
      this.sticky = sticky;
      this.maxHeight = maxHeight;
    }
  }

  private static final class UpdateExtraDataPayload {
    private final int sign;
    private final String type;
    private final String text;

    private UpdateExtraDataPayload(int sign, String type, String text) {
      this.sign = sign;
      this.type = type;
      this.text = text;
    }
  }

  private static final class UpdateInputValuePayload {
    private final int sign;
    private final String value;
    private final int selectionStart;
    private final int selectionEnd;

    private UpdateInputValuePayload(int sign, String value, int selectionStart, int selectionEnd) {
      this.sign = sign;
      this.value = value;
      this.selectionStart = selectionStart;
      this.selectionEnd = selectionEnd;
    }
  }

  private static final class InvokePayload {
    private final int sign;
    private final String method;
    private final Map<String, Object> params;

    private InvokePayload(int sign, String method, Map<String, Object> params) {
      this.sign = sign;
      this.method = method;
      this.params = params;
    }
  }

  private static final class TouchPayload {
    private final List<TouchPointPayload> points;

    private TouchPayload(List<TouchPointPayload> points) {
      this.points = points;
    }
  }
}
