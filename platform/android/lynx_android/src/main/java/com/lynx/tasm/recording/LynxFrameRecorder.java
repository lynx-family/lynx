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

  public static final int EVENT_LYNX_TOUCH_START = 201;
  public static final int EVENT_LYNX_TOUCH_MOVE = 202;
  public static final int EVENT_LYNX_TOUCH_END = 203;
  public static final int EVENT_LYNX_TOUCH_CANCEL = 204;

  private static final int MAX_CACHED_FRAMES_PER_INSTANCE = 1024;
  private static final long NANOS_PER_MILLIS = 1000000L;

  private static final LynxFrameRecorder sInstance = new LynxFrameRecorder();

  private final ExecutorService mExecutor =
      Executors.newSingleThreadExecutor(new RecorderThreadFactory());
  private final ConcurrentHashMap<Integer, Integer> mRecordingSessions = new ConcurrentHashMap<>();
  private final ConcurrentHashMap<Integer, Integer> mStoppingSessions = new ConcurrentHashMap<>();
  private final ConcurrentHashMap<Integer, FrameCache> mFrames = new ConcurrentHashMap<>();
  private final ConcurrentHashMap<Integer, FrameCallback> mFrameCallbacks =
      new ConcurrentHashMap<>();
  private final Map<Integer, PendingUIFrame> mPendingUIFrames = new HashMap<>();
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
    if (instanceId < 0) {
      return;
    }
    int sessionId = mNextSessionId.getAndIncrement();
    mRecordingSessions.put(instanceId, sessionId);
    mStoppingSessions.remove(instanceId);
    mFrames.put(instanceId, new FrameCache(MAX_CACHED_FRAMES_PER_INSTANCE));
    mExecutor.execute(() -> {
      if (!isRecordingSession(instanceId, sessionId)) {
        return;
      }
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
      return;
    }
    mStoppingSessions.put(instanceId, sessionId);
    mExecutor.execute(() -> {
      flushPendingUIFrameOnExecutor(instanceId, sessionId, SystemClock.uptimeMillis());
      mRecordingSessions.remove(instanceId, sessionId);
      mStoppingSessions.remove(instanceId, sessionId);
      if (!mRecordingSessions.containsKey(instanceId)) {
        mFrames.remove(instanceId);
      }
    });
  }

  public void recordCreateNode(int instanceId, int sign, String tagName, PropBundle bundle,
      ReadableMapBuffer initialStyles, boolean isFlatten, int nodeIndex) {
    long opTimeMillis = SystemClock.uptimeMillis();
    Map<String, Object> bundleSnapshot = snapshotPropBundle(bundle);
    Map<String, Object> initialStyleSnapshot = snapshotStyleMapBuffer(initialStyles);
    recordUIOperation(instanceId, EVENT_LYNX_UI_CREATE_NODE,
        new CreateNodePayload(
            sign, tagName, bundleSnapshot, initialStyleSnapshot, isFlatten, nodeIndex),
        opTimeMillis);
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
      float[] sticky, float maxHeight, int nodeIndex) {
    long opTimeMillis = SystemClock.uptimeMillis();
    recordUIOperation(instanceId, EVENT_LYNX_UI_UPDATE_LAYOUT,
        new UpdateLayoutPayload(sign, x, y, width, height, copyFloatArray(paddings),
            copyFloatArray(margins), copyFloatArray(borders), copyFloatArray(bounds),
            copyFloatArray(sticky), maxHeight, nodeIndex),
        opTimeMillis);
  }

  public void recordInvoke(
      int instanceId, int sign, String method, ReadableMap params, long context, int callback) {
    long opTimeMillis = SystemClock.uptimeMillis();
    Map<String, Object> paramsSnapshot = snapshotReadableMap(params);
    recordUIOperation(instanceId, EVENT_LYNX_UI_INVOKE,
        new InvokePayload(sign, method, paramsSnapshot, context, callback), opTimeMillis);
  }

  public void recordTouch(
      int instanceId, int eventType, List<TouchPointPayload> points, long timestampMillis) {
    List<TouchPointPayload> pointSnapshot =
        points == null ? null : Collections.unmodifiableList(new ArrayList<>(points));
    recordFrame(instanceId, TRACK_TYPE_LYNX_TOUCH, eventType, new TouchPayload(pointSnapshot),
        timestampMillis);
  }

  private void recordUIOperation(int instanceId, int eventType, Object data, long opTimeMillis) {
    Integer sessionId = getActiveRecordingSession(instanceId);
    if (sessionId == null) {
      return;
    }
    mExecutor.execute(() -> {
      if (!isRecordingSession(instanceId, sessionId)) {
        return;
      }
      PendingUIFrame pendingFrame = mPendingUIFrames.get(instanceId);
      if (pendingFrame == null || pendingFrame.sessionId != sessionId) {
        pendingFrame = new PendingUIFrame(sessionId);
        mPendingUIFrames.put(instanceId, pendingFrame);
      }
      pendingFrame.operations.add(new UIOperationRecord(eventType, data, opTimeMillis));
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
        new UIFramePayload(operations), toUptimeMillis(frameTimeNanos));
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
      return result;
    }
    if (value instanceof CreateNodePayload) {
      CreateNodePayload payload = (CreateNodePayload) value;
      JSONObject result = new JSONObject();
      put(result, "sign", payload.sign);
      put(result, "tagName", payload.tagName);
      put(result, "isFlatten", payload.isFlatten);
      put(result, "nodeIndex", payload.nodeIndex);
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
      put(result, "tendToFlatten", payload.tendToFlatten);
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
      put(result, "nodeIndex", payload.nodeIndex);
      return result;
    }
    if (value instanceof InvokePayload) {
      InvokePayload payload = (InvokePayload) value;
      JSONObject result = new JSONObject();
      put(result, "sign", payload.sign);
      put(result, "method", payload.method);
      put(result, "params", payload.params);
      put(result, "context", payload.context);
      put(result, "callback", payload.callback);
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
          new UIFramePayload(operations), timestampMillis);
    }
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

    private UIFramePayload(List<UIOperationRecord> operations) {
      this.operations = operations;
    }
  }

  private static final class CreateNodePayload {
    private final int sign;
    private final String tagName;
    private final Map<String, Object> bundle;
    private final Map<String, Object> initialStyles;
    private final boolean isFlatten;
    private final int nodeIndex;

    private CreateNodePayload(int sign, String tagName, Map<String, Object> bundle,
        Map<String, Object> initialStyles, boolean isFlatten, int nodeIndex) {
      this.sign = sign;
      this.tagName = tagName;
      this.bundle = bundle;
      this.initialStyles = initialStyles;
      this.isFlatten = isFlatten;
      this.nodeIndex = nodeIndex;
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
    private final boolean tendToFlatten;
    private final Map<String, Object> bundle;
    private final Map<String, Object> styles;

    private UpdatePropsPayload(
        int sign, boolean tendToFlatten, Map<String, Object> bundle, Map<String, Object> styles) {
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
    private final int nodeIndex;

    private UpdateLayoutPayload(int sign, float x, float y, float width, float height,
        float[] paddings, float[] margins, float[] borders, float[] bounds, float[] sticky,
        float maxHeight, int nodeIndex) {
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
      this.nodeIndex = nodeIndex;
    }
  }

  private static final class InvokePayload {
    private final int sign;
    private final String method;
    private final Map<String, Object> params;
    private final long context;
    private final int callback;

    private InvokePayload(
        int sign, String method, Map<String, Object> params, long context, int callback) {
      this.sign = sign;
      this.method = method;
      this.params = params;
      this.context = context;
      this.callback = callback;
    }
  }

  private static final class TouchPayload {
    private final List<TouchPointPayload> points;

    private TouchPayload(List<TouchPointPayload> points) {
      this.points = points;
    }
  }
}
