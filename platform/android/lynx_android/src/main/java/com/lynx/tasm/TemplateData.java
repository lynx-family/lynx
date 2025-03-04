// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import android.text.TextUtils;
import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.common.LepusBuffer;
import com.lynx.tasm.common.NullableConcurrentHashMap;
import com.lynx.tasm.core.LynxThreadPool;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicBoolean;
import org.json.JSONArray;
import org.json.JSONObject;

public final class TemplateData {
  private static final String TAG = "LynxTemplateData";
  private static final String TRACE_TEMPLATE_DATA_FROM_MAP = "TemplateData.FromMap";
  private static final String TRACE_TEMPLATE_DATA_FROM_STRING = "TemplateData.FromString";

  /**
   * Record Template Data Update Actions;
   */
  static class UpdateAction {
    final private ActionType mType;

    private ByteBuffer mBuffer = null;
    private String mJsonString = null;

    private long mNativeData = 0;

    UpdateAction(ByteBuffer buffer) {
      mType = ActionType.BYTE_BUFFER;
      mBuffer = buffer;
    }

    UpdateAction(String json) {
      mType = ActionType.STRING_DATA;
      mJsonString = json;
    }

    UpdateAction(long nativeData) {
      mType = ActionType.NATIVE_DATA;
      mNativeData = nativeData;
    }

    ActionType getType() {
      return mType;
    }

    String getJsonString() {
      return mJsonString;
    }

    ByteBuffer getByteBuffer() {
      return mBuffer;
    }

    long getNativeData() {
      return mNativeData;
    }

    @Override
    protected void finalize() throws Throwable {
      super.finalize();
      if (mNativeData != 0) {
        nativeReleaseData(mNativeData);
      }
    }
  }

  private volatile long mNativeData;
  private Map<String, Object> mData;
  private String mProcessorName;
  private volatile boolean mIsConcurrent;
  private boolean readOnly = false;
  volatile long mJsNativeData;
  private final AtomicBoolean mConsumed = new AtomicBoolean(false);

  enum ActionType {
    STRING_DATA,
    BYTE_BUFFER,
    NATIVE_DATA,
  }
  List<UpdateAction> mUpdateActions = new ArrayList<>();

  /**
   * Create a templateData from Map with String keys.
   * This method may has performance issue.
   */
  @NonNull
  public static TemplateData fromMap(Map<String, Object> data) {
    TraceEvent.beginSection(TRACE_TEMPLATE_DATA_FROM_MAP);
    TemplateData result = new TemplateData(data);
    TraceEvent.endSection(TRACE_TEMPLATE_DATA_FROM_MAP);
    return result;
  }

  /**
   * Create an empty TemplateData.
   */
  @NonNull
  public static TemplateData empty() {
    return new TemplateData();
  }

  /**
   * Create a TemplateData from json formatted String.
   */
  @NonNull
  public static TemplateData fromString(String json) {
    TraceEvent.beginSection(TRACE_TEMPLATE_DATA_FROM_STRING);
    TemplateData result = new TemplateData(json);
    TraceEvent.endSection(TRACE_TEMPLATE_DATA_FROM_STRING);
    return result;
  }

  private synchronized void addUpdateActions(List<UpdateAction> actions) {
    if (actions == null) {
      return;
    }
    if (mUpdateActions == null) {
      mUpdateActions = new ArrayList<>();
    }
    mUpdateActions.addAll(actions);
  }

  private synchronized void addUpdateAction(UpdateAction action) {
    if (action == null) {
      return;
    }
    if (mUpdateActions == null) {
      mUpdateActions = new ArrayList<>();
    }
    mUpdateActions.add(action);
  }

  private synchronized List<UpdateAction> getUpdateActionsWithJsNativeData() {
    List<UpdateAction> actions = new ArrayList<>();
    if (mJsNativeData != 0) {
      actions.add(new UpdateAction(nativeShallowCopy(mJsNativeData)));
    }
    actions.addAll(mUpdateActions);
    // When `getUpdateActionsWithNativeData` is called, it indicates that the current `TemplateData`
    // has been deep cloned or updated to another `TemplateData`. At this point, it is highly likely
    // that the current `TemplateData` will not be consumed by `LynxView`. Therefore, it is
    // necessary to actively call `consumeUpdateActions` to avoid OOM.
    consumeUpdateActions();
    return actions;
  }

  private synchronized List<UpdateAction> obtainUpdateActions() {
    List<UpdateAction> actions = new ArrayList<>(mUpdateActions);
    mUpdateActions.clear();
    return actions;
  }

  public void markConcurrent() {
    mIsConcurrent = true;
  }

  public long getNativePtr() {
    return mNativeData;
  }

  public void removeKey(String key) {
    put(key, null);
  }

  public Map<Object, Object> toMap() {
    if (!checkIfEnvPrepared()) {
      LLog.w(TAG, "toMap failed since env not ready.");
      return null;
    }

    flush();
    if (mNativeData == 0) {
      LLog.w(TAG, "toMap failed since mNativeData == 0.");
      return null;
    }

    Object data = nativeGetData(mNativeData);
    Map result = new HashMap<>();
    if (data instanceof Map) {
      result.putAll((Map) data);
    }
    return result;
  }

  /**
   * Merge the current TemplateData with a new TemplateData.
   * @param diff
   */
  public void updateWithTemplateData(TemplateData diff) {
    if (diff == null) {
      return;
    }

    if (this == diff) {
      LLog.w(TAG, "can not update TemplateData with self");
      return;
    }

    if (readOnly) {
      LLog.w(TAG, "can not update readOnly TemplateData");
      return;
    }

    if (!checkIfEnvPrepared()) {
      LLog.w(TAG, "updateWithTemplateData failed since env not ready.");
      return;
    }
    if (mConsumed.get()) {
      LLog.w(
          TAG, "updateWithTemplateData to consumed TemplateData, this:" + this + ",diff:" + diff);
    }

    flush();
    diff.flush();
    // record diff's update actions
    addUpdateActions(diff.getUpdateActionsWithJsNativeData());

    if (diff.getNativePtr() != 0) {
      nativeMergeTemplateData(this.getNativePtr(), diff.getNativePtr());
    }
  }

  public void put(String key, Object value) {
    if (readOnly) {
      LLog.w(TAG, "can not update readOnly TemplateData");
      return;
    }
    if (mConsumed.get()) {
      LLog.w(TAG, "put data to consumed TemplateData,key:" + key);
    }
    putSafely(key, value);
  }

  public void updateData(Map<String, Object> diff) {
    if (readOnly) {
      LLog.e("Lynx", "can not update readOnly TemplateData");
      return;
    }
    if (mConsumed.get()) {
      LLog.w("Lynx", "updateData to consumed TemplateData, diff:" + diff.keySet());
    }
    putSafely(diff);
  }

  /**
   * @deprecated this method will be removed in later versions, you should use {@link #put(String,
   *     Object)} instead.
   */
  @Deprecated()
  public void updateData(String key, Object value) {
    put(key, value);
  }

  public synchronized void flush() {
    if (!checkIfEnvPrepared()) {
      LLog.w(TAG, "Env not ready!");
      return;
    }

    if (mData == null || mData.isEmpty()) {
      if (mNativeData == 0) {
        mNativeData = nativeCreateObject();
      }
      return;
    }

    ByteBuffer buffer = LepusBuffer.INSTANCE.encodeMessage(mData);
    mData.clear();

    if (buffer != null && buffer.position() > 0) {
      addUpdateAction(new UpdateAction(buffer));
      if (mNativeData == 0) {
        mNativeData = nativeParseData(buffer, buffer.position());
      } else {
        nativeUpdateData(mNativeData, buffer, buffer.position());
      }
    }
  }

  @Override
  protected void finalize() throws Throwable {
    recycle();
    recycleJsData();
    super.finalize();
  }

  // Since currently recycle() may be called actively after loading the template, and
  // getDataForJSThread() will be executed in the JS thread at this time, it is uncertain whether it
  // is completed. Therefore, to avoid thread safety issues, js data is not released in recycle()
  // and let js data be released in finalize().
  public synchronized void recycle() {
    if (checkIfEnvPrepared() && mNativeData != 0) {
      nativeReleaseData(mNativeData);
      mNativeData = 0;
    }
  }

  void recycleJsData() {
    if (checkIfEnvPrepared() && mJsNativeData != 0) {
      nativeReleaseData(mJsNativeData);
      mJsNativeData = 0;
    }
  }

  private void ensureInternalMap() {
    if (mData == null) {
      mData = mIsConcurrent ? new NullableConcurrentHashMap<String, Object>()
                            : new HashMap<String, Object>();
    }
  }

  private void putSafely(Map<String, Object> data) {
    if (data == null || data.isEmpty()) {
      return;
    }
    ensureInternalMap();
    mData.putAll(data);
  }

  private void putSafely(String key, Object value) {
    ensureInternalMap();
    mData.put(key, value);
  }

  private TemplateData() {
    LynxEnv.inst();
    mProcessorName = null;
  }

  private TemplateData(Map<String, Object> data) {
    if (data == null || data.isEmpty()) {
      return;
    }

    LynxEnv.inst();
    if (!checkIfEnvPrepared()) {
      // When `env` is not ready, put this data to mData. At this point, there is no need to record
      // the action since the corresponding action will be recorded when the map is flushed.
      putSafely(data);
      return;
    }

    ByteBuffer buffer = LepusBuffer.INSTANCE.encodeMessage(data);
    if (buffer == null || buffer.position() <= 0) {
      return;
    }

    mNativeData = nativeParseData(buffer, buffer.position());
    mProcessorName = null;
    addUpdateAction(new UpdateAction(buffer));
  }

  static List<Object> jsonArrayToList(JSONArray jsonArray) {
    List<Object> list = new ArrayList<>();
    for (int i = 0; i < jsonArray.length(); i++) {
      try {
        Object value = jsonArray.get(i);
        if (value instanceof JSONObject) {
          value = jsonToMap((JSONObject) value);
        } else if (value instanceof JSONArray) {
          value = jsonArrayToList((JSONArray) value);
        }
        list.add(value);
      } catch (Throwable e) {
        // Log specific error
        LLog.e(TAG, "Failed to parse JSONArray at index " + i + ": " + e.getMessage());
      }
    }
    return list;
  }

  static Map<String, Object> jsonToMap(JSONObject jsonObject) {
    Map<String, Object> map = new HashMap<>();
    try {
      Iterator<String> keys = jsonObject.keys();
      while (keys.hasNext()) {
        String key = keys.next();
        Object value = jsonObject.get(key);
        if (value instanceof JSONObject) {
          value = jsonToMap((JSONObject) value);
        } else if (value instanceof JSONArray) {
          value = jsonArrayToList((JSONArray) value);
        }
        map.put(key, value);
      }
    } catch (Throwable e) {
      // Log specific error
      LLog.e(TAG, "Failed to parse JSONObject: " + e.getMessage());
    }
    return map;
  }

  private TemplateData(String str) {
    if (TextUtils.isEmpty(str)) {
      return;
    }

    LynxEnv.inst();
    if (!checkIfEnvPrepared()) {
      // When `env` is not ready, convert the JSON string to a map and save it. At this point, there
      // is no need to record the action since the corresponding action will be recorded when the
      // map is flushed.
      try {
        JSONObject jsonObject = new JSONObject(str);
        putSafely(jsonToMap(jsonObject));
      } catch (Throwable e) {
        e.printStackTrace();
      }
      return;
    }

    mNativeData = nativeParseStringData(str);
    mProcessorName = null;
    addUpdateAction(new UpdateAction(str));
  }

  /**
   * check whether templateData is valid.
   * @return valid.
   */
  public boolean checkIsLegalData() {
    return mNativeData != 0;
  }

  private static boolean checkIfEnvPrepared() {
    return LynxEnv.inst().isNativeLibraryLoaded();
  }

  public void markState(String name) {
    mProcessorName = name;
  }

  public String processorName() {
    return mProcessorName;
  }

  /**
   * Deep Clone a copy for a using TemplateData.
   * often used when using one TemplateData between multi LynxViews.
   *
   * @return The cloned TemplateData
   */
  public TemplateData deepClone() {
    if (!checkIfEnvPrepared()) {
      LLog.e(TAG, "deepClone failed since env not ready!");
      return TemplateData.empty();
    }

    this.flush();
    TemplateData data = TemplateData.empty();
    if (this.mNativeData != 0) {
      data.mNativeData = nativeClone(this.mNativeData);
    }
    // Since it has already been flushed, `mData` must be empty, so `mData` will no longer be
    // copied.
    data.mProcessorName = this.mProcessorName;
    data.readOnly = this.readOnly;
    data.mIsConcurrent = this.mIsConcurrent;

    // clone data actions
    data.addUpdateActions(this.getUpdateActionsWithJsNativeData());
    return data;
  }

  /**
   * TemplateData will be sync to Native. For Thread-Safety, we will clone the value in Native Side.
   * In some case, this may result in performance-loss, If your data won't change any more, Please
   * call this method to mark value Read-Only, so we'll no longer clone the value any more to
   * improve performance.
   */
  public void markReadOnly() {
    this.readOnly = true;
  }

  public boolean isReadOnly() {
    return this.readOnly;
  }

  public boolean isEmpty() {
    return this.mNativeData == 0;
  }

  void markConsumed() {
    mConsumed.set(true);
  }

  // Just for android unit test.
  static long createNativeTemplateData(TemplateData data) {
    data.flush();
    return nativeCreateTemplateData(
        data.getNativePtr(), data.isReadOnly(), data.processorName(), data);
  }

  // Just for android unit test.
  static void releaseNativeTemplateData(long ptr) {
    nativeReleaseTemplateData(ptr);
  }

  @CalledByNative
  private void consumeUpdateActions() {
    LynxThreadPool.getAsyncServiceExecutor().execute(new Runnable() {
      @Override
      public void run() {
        getDataForJSThread();
      }
    });
  }

  @CalledByNative
  synchronized long getDataForJSThread() {
    // Init mJsNativeData or update mUpdateActions to mJsNativeData
    List<UpdateAction> actions = obtainUpdateActions();
    if (actions.isEmpty()) {
      return mJsNativeData;
    }

    if (mJsNativeData == 0) {
      mJsNativeData = nativeCreateObject();
    }

    for (UpdateAction action : actions) {
      if (action.getType() == ActionType.STRING_DATA) {
        String jsonString = action.getJsonString();
        if (TextUtils.isEmpty(jsonString)) {
          continue;
        }
        long mergePtr = nativeParseStringData(jsonString);
        nativeMergeTemplateData(mJsNativeData, mergePtr);
        nativeReleaseData(mergePtr);
      } else if (action.getType() == ActionType.NATIVE_DATA) {
        nativeMergeTemplateData(mJsNativeData, action.getNativeData());
      } else {
        ByteBuffer buffer = action.getByteBuffer();
        if (buffer == null || buffer.position() == 0) {
          continue;
        }
        nativeUpdateData(mJsNativeData, action.getByteBuffer(), action.getByteBuffer().position());
      }
    }
    return mJsNativeData;
  }

  @CalledByNative
  private static Object decodeByteBuffer(ByteBuffer buffer) {
    if (buffer != null) {
      return LepusBuffer.INSTANCE.decodeMessage(buffer);
    }
    return null;
  }

  private static native long nativeParseStringData(String data);
  private static native void nativeReleaseData(long data);
  private static native long nativeParseData(ByteBuffer data, int length);
  private static native void nativeUpdateData(long nativePtr, ByteBuffer data, int length);
  static native Object nativeGetData(long nativePtr);
  private static native long nativeClone(long nativePtr);
  private static native long nativeShallowCopy(long nativePtr);
  private static native long nativeCreateObject();
  static native void nativeMergeTemplateData(long destPtr, long srcPtr);

  private static native long nativeCreateTemplateData(
      long nativePtr, boolean readOnly, String name, Object templateData);

  private static native void nativeReleaseTemplateData(long nativePtr);
}
