// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import android.text.TextUtils;
import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.base.trace.TraceEventDef;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.common.LepusBuffer;
import com.lynx.tasm.common.NullableConcurrentHashMap;
import com.lynx.tasm.core.LynxThreadPool;
import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicBoolean;
import org.json.JSONArray;
import org.json.JSONObject;

/**
 * @apidoc
 *
 * @brief TemplateData is a data structure provided by Lynx.
 * It is used to store the data types accepted by Lynx at runtime
 * and can be constructed using either a string or a map type.
 */
public final class TemplateData {
  private static final String TAG = "LynxTemplateData";

  /**
   * Record Template Data Update Actions;
   */
  static class UpdateAction {
    private ActionType mType;
    private ByteBuffer mBuffer = null;
    private String mJsonString = null;
    private String mKey = null;

    private long mNativeData = 0;

    private UpdateAction() {}

    static UpdateAction buildBufferAction(ByteBuffer buffer) {
      UpdateAction action = new UpdateAction();
      action.mType = ActionType.BYTE_BUFFER;
      action.mBuffer = buffer;
      return action;
    }

    static UpdateAction buildStringAction(String json) {
      UpdateAction action = new UpdateAction();
      action.mType = ActionType.STRING_DATA;
      action.mJsonString = json;
      return action;
    }

    static UpdateAction buildNativeAction(long nativeData) {
      UpdateAction action = new UpdateAction();
      action.mType = ActionType.NATIVE_DATA;
      action.mNativeData = nativeData;
      return action;
    }

    static UpdateAction buildRemoveAction(String key) {
      UpdateAction action = new UpdateAction();
      action.mType = ActionType.REMOVE_DATA;
      action.mKey = key;
      return action;
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

    @CalledByNative
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
  private volatile Map<String, Object> mData;
  private String mProcessorName;
  private final AtomicBoolean mIsConcurrent = new AtomicBoolean(false);
  private volatile boolean readOnly = false; // volatile ensures visibility
  // To garantee multi-thread safety, any access of mJsNativeData must
  // by synchronized.
  volatile long mJsNativeData;
  private volatile boolean mEnableJSData = true; // volatile ensures visibility
  private final AtomicBoolean mConsumed = new AtomicBoolean(false);
  private WeakReference<LynxContext> weakContext;

  private final Object mUpdateActionsLock = new Object();
  private final Object mDataLock = new Object();
  private final Object mJsNativeDataLock = new Object();
  private final Object mNativeDataLock = new Object();

  enum ActionType {
    STRING_DATA,
    BYTE_BUFFER,
    NATIVE_DATA,
    REMOVE_DATA,
  }
  List<UpdateAction> mUpdateActions = new ArrayList<>();

  /**
   * @apidoc
   * @brief Input data in key-value format and return the parsed `TemplateData` object.
   * @param data `Map` type data.
   * @return `TemplateData` constructed by `data`.
   */
  @NonNull
  public static TemplateData fromMap(Map<String, Object> data) {
    TraceEvent.beginSection(TraceEventDef.TEMPLATE_DATA_FROM_MAP);
    TemplateData result = new TemplateData(data);
    TraceEvent.endSection(TraceEventDef.TEMPLATE_DATA_FROM_MAP);
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
   * @apidoc
   * @brief Input data in json and return the parsed `TemplateData` object.
   * @param json `String` type data in json format.
   * @return `TemplateData` constructed by `json`.
   */
  @NonNull
  public static TemplateData fromString(String json) {
    TraceEvent.beginSection(TraceEventDef.TEMPLATE_DATA_FROM_STRING);
    TemplateData result = new TemplateData(json);
    TraceEvent.endSection(TraceEventDef.TEMPLATE_DATA_FROM_STRING);
    return result;
  }

  private void addUpdateActions(List<UpdateAction> actions) {
    if (actions == null) {
      return;
    }
    if (!mEnableJSData) {
      return;
    }
    synchronized (mUpdateActionsLock) {
      mUpdateActions.addAll(actions);
    }
  }

  private void addUpdateAction(UpdateAction action) {
    if (action == null) {
      return;
    }

    if (!mEnableJSData) {
      return;
    }
    synchronized (mUpdateActionsLock) {
      mUpdateActions.add(action);
    }
  }

  private List<UpdateAction> getUpdateActionsWithJsNativeData() {
    if (!mEnableJSData) {
      return null;
    }
    List<UpdateAction> actions = new ArrayList<>();

    // Use mJsNativeDataLock when accessing mJsNativeData
    synchronized (mJsNativeDataLock) {
      if (mJsNativeData != 0) {
        actions.add(UpdateAction.buildNativeAction(nativeShallowCopy(mJsNativeData)));
      }
    }

    synchronized (mUpdateActionsLock) {
      actions.addAll(mUpdateActions);
    }

    // When `getUpdateActionsWithNativeData` is called, it indicates that the current `TemplateData`
    // has been deep cloned or updated to another `TemplateData`. At this point, it is highly likely
    // that the current `TemplateData` will not be consumed by `LynxView`. Therefore, it is
    // necessary to actively call `consumeUpdateActions` to avoid OOM.
    consumeUpdateActions();
    return actions;
  }

  private List<UpdateAction> obtainUpdateActions() {
    synchronized (mUpdateActionsLock) {
      List<UpdateAction> actions = new ArrayList<>(mUpdateActions);
      mUpdateActions.clear();
      return actions;
    }
  }

  public void markConcurrent() {
    mIsConcurrent.set(true);
  }

  public long getNativePtr() {
    return mNativeData;
  }

  /**
   * Use {@link com.lynx.tasm.TemplateData#remove(String)} instead
   */
  @Deprecated
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

  public void remove(String key) {
    if (!checkIfEnvPrepared()) {
      LLog.e(TAG, "remove failed since env not ready!");
      return;
    }
    if (readOnly) {
      LLog.w(TAG, "can not remove readOnly TemplateData");
      return;
    }
    if (mConsumed.get()) {
      LLog.w(TAG, "put data to consumed TemplateData,key:" + key);
    }
    addUpdateAction(UpdateAction.buildRemoveAction(key));
    if (mNativeData != 0) {
      nativeRemoveData(mNativeData, key);
    }
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

  public void flush() {
    if (!checkIfEnvPrepared()) {
      LLog.w(TAG, "Env not ready!");
      return;
    }

    Map<String, Object> dataToFlush;
    ByteBuffer buffer = null;

    // First get and clear data under mDataLock protection
    synchronized (mDataLock) {
      if (mData == null || mData.isEmpty()) {
        synchronized (mNativeDataLock) {
          if (mNativeData == 0) {
            mNativeData = nativeCreateObject();
          }
        }
        return;
      }

      dataToFlush = new HashMap<>(mData);
      mData.clear();
    }

    // Perform time-consuming operations outside the lock
    buffer = LepusBuffer.INSTANCE.encodeMessage(dataToFlush);

    if (buffer != null && buffer.position() > 0) {
      LLog.i(TAG, "flush data." + this);
      // Need mUpdateActionsLock to add update actions
      addUpdateAction(UpdateAction.buildBufferAction(buffer));

      // Need mNativeDataLock to update native data
      synchronized (mNativeDataLock) {
        if (mNativeData == 0) {
          mNativeData = nativeParseData(buffer, buffer.position());
        } else {
          nativeUpdateData(mNativeData, buffer, buffer.position());
        }
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
  public void recycle() {
    // Try making sure that the native pointer
    // is released on engine thread.
    if (weakContext != null) {
      LynxContext context = weakContext.get();
      if (context != null) {
        context.runOnTasmThread(new Runnable() {
          @Override
          public void run() {
            releaseNativeData();
          }
        });
        return;
      }
    }
    releaseNativeData();
  }

  private void releaseNativeData() {
    if (mNativeData == 0) {
      return;
    }
    synchronized (mNativeDataLock) {
      if (checkIfEnvPrepared() && mNativeData != 0) {
        nativeReleaseData(mNativeData);
        mNativeData = 0;
      }
    }
  }

  private void releaseJsData() {
    if (mJsNativeData == 0) {
      return;
    }
    synchronized (mJsNativeDataLock) {
      if (checkIfEnvPrepared() && mJsNativeData != 0) {
        nativeReleaseData(mJsNativeData);
        mJsNativeData = 0;
      }
    }
  }

  void recycleJsData() {
    if (weakContext != null) {
      LynxContext context = weakContext.get();
      if (context != null) {
        context.runOnTasmThread(new Runnable() {
          @Override
          public void run() {
            releaseJsData();
          }
        });
        return;
      }
    }
    releaseJsData();
  }

  private void ensureInternalMap() {
    if (mData == null) {
      synchronized (mDataLock) {
        if (mData == null) {
          mData = mIsConcurrent.get() ? new NullableConcurrentHashMap<String, Object>()
                                      : new HashMap<String, Object>();
        }
      }
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
    addUpdateAction(UpdateAction.buildBufferAction(buffer));
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
    addUpdateAction(UpdateAction.buildStringAction(str));
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

  /**
   * @apidoc
   * @brief Mark the current TemplateData with the associated dataProcessor name.
   * When this TemplateData is used in
   * [UpdateData](/api/lynx-native-api/lynx-view/update-meta-data), the corresponding dataProcessor
   * will be found based on this name for data preprocessing.
   * @param name Marked name.
   */
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
    data.mIsConcurrent.set(this.mIsConcurrent.get());

    // clone data actions
    data.addUpdateActions(this.getUpdateActionsWithJsNativeData());
    return data;
  }

  /**
   * Shallow Clone a copy for a using TemplateData.
   * often used when using one TemplateData between multi Thread in a LynxView.
   *
   * @return The cloned TemplateData
   */
  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public TemplateData shallowClone() {
    TraceEvent.beginSection(TraceEventDef.TEMPLATE_DATA_SHALLOW_CLONE);
    if (!checkIfEnvPrepared()) {
      LLog.e(TAG, "shallowClone failed since env not ready!");
      TraceEvent.endSection(TraceEventDef.TEMPLATE_DATA_SHALLOW_CLONE);
      return TemplateData.empty();
    }

    this.flush();
    TemplateData data = TemplateData.empty();
    if (this.mNativeData != 0) {
      data.mNativeData = nativeShallowCopy(this.mNativeData);
    }
    // Since it has already been flushed, `mData` must be empty, so `mData` will no longer be
    // copied.
    data.mProcessorName = this.mProcessorName;
    data.readOnly = this.readOnly;
    data.mIsConcurrent.set(this.mIsConcurrent.get());

    // clone data actions
    data.addUpdateActions(this.getUpdateActionsWithJsNativeData());
    TraceEvent.endSection(TraceEventDef.TEMPLATE_DATA_SHALLOW_CLONE);
    return data;
  }

  void bindContext(LynxContext context) {
    this.weakContext = new WeakReference<>(context);
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

  void setEnableJSData(boolean enableJSData) {
    this.mEnableJSData = enableJSData;
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
        getDataForJSThreadInner();
      }
    });
  }

  long getDataForJSThreadInner() {
    if (!mEnableJSData) {
      return 0;
    }

    List<UpdateAction> actions = obtainUpdateActions();
    if (actions.isEmpty()) {
      // If there are no update actions, check and return mJsNativeData
      synchronized (mJsNativeDataLock) {
        return mJsNativeData;
      }
    }

    // Update mJsNativeData inside the lock, but minimize operations within the lock
    synchronized (mJsNativeDataLock) {
      if (mJsNativeData == 0) {
        mJsNativeData = nativeCreateObject();
      }

      for (UpdateAction action : actions) {
        switch (action.getType()) {
          case STRING_DATA: {
            String jsonString = action.getJsonString();
            if (TextUtils.isEmpty(jsonString)) {
              continue;
            }
            // Perform native operations inside the lock, but create temporary objects outside the
            // lock
            long mergePtr = 0;
            try {
              mergePtr = nativeParseStringData(jsonString);
              nativeMergeTemplateData(mJsNativeData, mergePtr);
            } finally {
              if (mergePtr != 0) {
                nativeReleaseData(mergePtr);
              }
            }
          } break;
          case NATIVE_DATA: {
            nativeMergeTemplateData(mJsNativeData, action.getNativeData());
          } break;
          case BYTE_BUFFER: {
            ByteBuffer buffer = action.getByteBuffer();
            if (buffer == null || buffer.position() == 0) {
              continue;
            }
            // Cache position to avoid multiple method calls
            int position = buffer.position();
            nativeUpdateData(mJsNativeData, buffer, position);
          } break;
          case REMOVE_DATA: {
            nativeRemoveData(mJsNativeData, action.mKey);
          } break;
          default: {
            LLog.e(TAG, "undefined action type: " + action.getType());
          } break;
        }
      }
      return mJsNativeData;
    }
  }

  @CalledByNative
  long getDataForJSThread() {
    return getDataForJSThreadInner();
  }

  // Used to copy a new templateData that contains updateActions only.
  @CalledByNative
  private TemplateData getTemplateDataForJSThread() {
    TemplateData data = TemplateData.empty();
    data.mEnableJSData = true;
    if (mJsNativeData != 0) {
      data.mJsNativeData = nativeClone(mJsNativeData);
    }
    data.addUpdateActions(obtainUpdateActions());
    return data;
  }

  @CalledByNative
  long getNativeTemplateData() {
    return this.mNativeData;
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
  private static native void nativeRemoveData(long nativePtr, String key);
  static native Object nativeGetData(long nativePtr);
  private static native long nativeClone(long nativePtr);
  private static native long nativeShallowCopy(long nativePtr);
  private static native long nativeCreateObject();
  static native void nativeMergeTemplateData(long destPtr, long srcPtr);

  private static native long nativeCreateTemplateData(
      long nativePtr, boolean readOnly, String name, Object templateData);

  private static native void nativeReleaseTemplateData(long nativePtr);
}
