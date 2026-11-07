// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.memory;

import android.util.Log;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.LynxGlobalMemoryUsageCallback;
import com.lynx.tasm.LynxGlobalMemoryUsageResult;
import com.lynx.tasm.LynxInstanceMemoryUsage;
import com.lynx.tasm.LynxMemoryCollectionStatus;
import com.lynx.tasm.LynxMemoryUsageQuery;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.performance.memory.MemoryRecord;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicBoolean;
import org.json.JSONException;
import org.json.JSONObject;

public final class MemoryUsageResultSerializer {
  private static final String TAG = "MemoryUsageResultSerializer";
  private static final String EMPTY_RESULT_JSON = "{}";

  private MemoryUsageResultSerializer() {}

  public interface ResultCallback {
    void onResult(@NonNull String resultJson, @NonNull String errorMessage);
  }

  public static void queryAllMemoryUsage(long timeoutMs, @NonNull ResultCallback callback) {
    AtomicBoolean didCallback = new AtomicBoolean(false);
    try {
      LynxMemoryUsageQuery.inst().queryLynxGlobalMemoryUsageAsync(
          new LynxGlobalMemoryUsageCallback() {
            @Override
            public void onResult(@NonNull LynxGlobalMemoryUsageResult result) {
              String resultJson = EMPTY_RESULT_JSON;
              String errorMessage = "";
              try {
                resultJson = toJson(result).toString();
              } catch (Throwable throwable) {
                errorMessage =
                    failureMessage("Failed to serialize Lynx memory usage result", throwable);
              }
              complete(callback, didCallback, resultJson, errorMessage);
            }
          },
          timeoutMs);
    } catch (Throwable throwable) {
      complete(callback, didCallback, EMPTY_RESULT_JSON,
          failureMessage("Failed to query Lynx memory usage", throwable));
    }
  }

  @NonNull
  static JSONObject toJson(@NonNull LynxGlobalMemoryUsageResult result) throws JSONException {
    JSONObject json = new JSONObject();
    json.put("collectionStartMs", result.getCollectionStartMs());
    json.put("collectionStatus", statusToString(result.getCollectionStatus()));
    json.put("collectionDurationMs", result.getCollectionDurationMs());
    json.put("collectionTimeoutMs", result.getCollectionTimeoutMs());
    json.put("expectedInstanceCount", result.getExpectedInstanceCount());
    json.put("completedInstanceCount", result.getCompletedInstanceCount());
    json.put("totalBytes", result.getTotalBytes());
    json.put("appBytes", result.getAppBytes());
    json.put("ratioToApp", result.getRatioToApp());
    json.put("elementBytes", result.getElementBytes());
    json.put("elementNodeCount", result.getElementNodeCount());
    json.put("viewBytes", result.getViewBytes());
    json.put("mainThreadRuntimeBytes", result.getMainThreadRuntimeBytes());
    json.put("backgroundThreadRuntimeBytes", result.getBackgroundThreadRuntimeBytes());

    org.json.JSONArray instancesJson = new org.json.JSONArray();
    List<LynxInstanceMemoryUsage> instances = result.getInstances();
    if (instances != null) {
      for (LynxInstanceMemoryUsage instance : instances) {
        if (instance != null) {
          instancesJson.put(instanceToJson(instance));
        }
      }
    }
    json.put("instances", instancesJson);
    return json;
  }

  @NonNull
  private static JSONObject instanceToJson(@NonNull LynxInstanceMemoryUsage instance)
      throws JSONException {
    JSONObject json = new JSONObject();
    json.put("instanceId", instance.getInstanceId());
    json.put("pageId", safeString(instance.getPageId()));
    json.put("url", safeString(instance.getUrl()));
    json.put("totalBytes", instance.getTotalBytes());
    json.put("elementBytes", instance.getElementBytes());
    json.put("elementNodeCount", instance.getElementNodeCount());
    json.put("viewBytes", instance.getViewBytes());
    json.put("viewDetail", viewDetailToJson(instance.getViewDetail()));
    json.put("mainThreadRuntimeBytes", instance.getMainThreadRuntimeBytes());
    json.put("backgroundThreadRuntimeBytes", instance.getBackgroundThreadRuntimeBytes());
    json.put("btsRuntimeGroupId", safeString(instance.getBtsRuntimeGroupId()));
    return json;
  }

  @NonNull
  private static JSONObject viewDetailToJson(@Nullable Map<String, MemoryRecord> records)
      throws JSONException {
    JSONObject json = new JSONObject();
    if (records == null || records.isEmpty()) {
      return json;
    }
    for (Map.Entry<String, MemoryRecord> entry : records.entrySet()) {
      String key = entry.getKey();
      if (!isValidJsonKey(key)) {
        continue;
      }
      json.put(key, memoryRecordToJson(entry.getValue()));
    }
    return json;
  }

  @NonNull
  private static JSONObject memoryRecordToJson(@Nullable MemoryRecord record) throws JSONException {
    JSONObject json = new JSONObject();
    if (record == null) {
      json.put("category", "");
      json.put("sizeBytes", 0L);
      json.put("instanceCount", 0);
      json.put("detail", new JSONObject());
      return json;
    }
    json.put("category", safeString(record.getCategory()));
    json.put("sizeBytes", record.mSizeBytes);
    json.put("instanceCount", record.mInstanceCount);
    json.put("detail", stringMapToJson(record.mDetail));
    return json;
  }

  @NonNull
  private static JSONObject stringMapToJson(@Nullable Map<String, String> map)
      throws JSONException {
    JSONObject json = new JSONObject();
    if (map == null || map.isEmpty()) {
      return json;
    }
    for (Map.Entry<String, String> entry : map.entrySet()) {
      String key = entry.getKey();
      if (!isValidJsonKey(key)) {
        continue;
      }
      json.put(key, safeString(entry.getValue()));
    }
    return json;
  }

  @NonNull
  private static String statusToString(@Nullable LynxMemoryCollectionStatus status) {
    if (status == LynxMemoryCollectionStatus.TIMEOUT) {
      return "timeout";
    }
    return "completed";
  }

  @NonNull
  private static String failureMessage(@NonNull String prefix, @NonNull Throwable throwable) {
    String message = throwable.getMessage();
    if (message == null || message.isEmpty()) {
      message = throwable.getClass().getSimpleName();
    }
    return prefix + ": " + message;
  }

  @NonNull
  private static String safeString(@Nullable String value) {
    return value == null ? "" : value;
  }

  private static boolean isValidJsonKey(@Nullable String key) {
    return key != null && !key.isEmpty();
  }

  private static void complete(@NonNull ResultCallback callback, @NonNull AtomicBoolean didCallback,
      @NonNull String resultJson, @NonNull String errorMessage) {
    if (didCallback.compareAndSet(false, true)) {
      try {
        callback.onResult(resultJson, errorMessage);
      } catch (Throwable throwable) {
        LLog.e(TAG,
            "Failed to deliver Lynx memory usage result: " + Log.getStackTraceString(throwable));
      }
    }
  }
}
