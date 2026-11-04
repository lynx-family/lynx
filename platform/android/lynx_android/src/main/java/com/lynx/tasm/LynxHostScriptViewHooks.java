// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import androidx.annotation.Nullable;
import androidx.annotation.VisibleForTesting;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.utils.UIThreadUtils;
import java.nio.charset.StandardCharsets;
import java.util.IdentityHashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/** Debug-engine View notifications and UI-domain interception entries. */
final class LynxHostScriptViewHooks {
  private static final String TAG = "LynxHostScriptViewHooks";
  // Where a load reads its template from; native code names them in events.
  private static final int SOURCE_URL = 0;
  private static final int SOURCE_BYTES = 1;
  private static final int SOURCE_BUFFER = 2;
  private static final int SOURCE_BUNDLE = 3;
  // A new token separates queued notifications from earlier enabled periods.
  private static volatile Object sEnabledToken;
  private static long sNextId;

  private LynxHostScriptViewHooks() {}

  @CalledByNative
  private static void setEnabled(boolean enabled) {
    UIThreadUtils.assertOnUiThread();
    if (!enabled) {
      sEnabledToken = null;
    } else if (sEnabledToken == null) {
      sEnabledToken = new Object();
    }
  }

  static boolean isEnabled() {
    return sEnabledToken != null;
  }

  static void onCreated(LynxView view) {
    postCreationNotification(() -> {
      String url = view.getTemplateUrl();
      nativeDispatchCreated(++sNextId, url == null ? "" : url);
    });
  }

  @VisibleForTesting
  static void postCreationNotification(Runnable notification) {
    Object token = sEnabledToken;
    if (token == null) {
      return;
    }
    UIThreadUtils.runOnUiThreadImmediately(() -> {
      if (sEnabledToken != token) {
        return;
      }
      notification.run();
    });
  }

  /**
   * Dispatches view.loadTemplate before the template is fetched. SSR loads are not covered.
   */
  static void beforeLoadTemplate(LynxTemplateRender render, LynxLoadMeta meta) {
    if (!isEnabled() || meta.loadMode == LynxLoadMode.RENDER_SSR
        || meta.loadMode == LynxLoadMode.HYDRATE_SSR) {
      return;
    }
    // Mirrors the source precedence of the load itself.
    int source = SOURCE_URL;
    int size = 0;
    if (meta.isBundleValid()) {
      source = SOURCE_BUNDLE;
    } else if (meta.isByteBufferValid()) {
      source = SOURCE_BUFFER;
      size = meta.byteBuffer.capacity();
    } else if (meta.isBinaryValid()) {
      source = SOURCE_BYTES;
      size = meta.binaryData.length;
    }
    String url = meta.url == null ? "" : meta.url;
    Object[] patch = nativeDispatchLoadTemplate(render.getLogContextSnapshot().viewId,
        url.getBytes(StandardCharsets.UTF_8), source, size, nativePtr(meta.initialData),
        nativePtr(meta.globalProps));
    if (patch == null) {
      return;
    }
    try {
      applyLoadPatch(meta, new JSONObject(new String((byte[]) patch[0], StandardCharsets.UTF_8)),
          (byte[]) patch[1]);
    } catch (JSONException e) {
      LLog.e(TAG, "view.loadTemplate patch was not applied: " + e.getMessage());
    }
  }

  /** Dispatches view.updateMetaData before platform caches consume the update fragments. */
  static LynxUpdateMeta beforeUpdateMetaData(LynxTemplateRender render, LynxUpdateMeta meta) {
    if (!isEnabled()) {
      return meta;
    }
    byte[] patch = nativeDispatchUpdateMetaData(render.getLogContextSnapshot().viewId,
        nativePtr(meta.getUpdatedData()), nativePtr(meta.getUpdatedGlobalProps()));
    if (patch == null) {
      return meta;
    }
    try {
      return applyUpdatePatch(meta, new JSONObject(new String(patch, StandardCharsets.UTF_8)));
    } catch (JSONException e) {
      LLog.e(TAG, "view.updateMetaData patch was not applied: " + e.getMessage());
      return meta;
    }
  }

  private static LynxUpdateMeta applyUpdatePatch(LynxUpdateMeta meta, JSONObject values)
      throws JSONException {
    TemplateData data = meta.getUpdatedData();
    TemplateData globalProps = meta.getUpdatedGlobalProps();
    return new LynxUpdateMeta.Builder()
        .setUpdatedData(
            values.has("updateData") ? templateData(values.get("updateData"), data) : data)
        .setUpdatedGlobalProps(values.has("globalProps")
                ? templateData(values.get("globalProps"), globalProps)
                : globalProps)
        .build();
  }

  private static void applyLoadPatch(LynxLoadMeta meta, JSONObject patch, byte[] bytes)
      throws JSONException {
    // Decode every replacement before mutating the caller's request. A malformed patch must
    // leave all original fields intact, including its template source.
    String url = patch.has("url") ? patch.getString("url") : meta.url;
    // Native code only reports a source that differs from the original.
    JSONObject source = patch.optJSONObject("source");
    byte[] binaryData = source == null ? meta.binaryData
                                       : ("bytes".equals(source.getString("kind")) ? bytes : null);
    TemplateData initialData = patch.has("initialData")
        ? templateData(patch.get("initialData"), meta.initialData)
        : meta.initialData;
    TemplateData globalProps = patch.has("globalProps")
        ? templateData(patch.get("globalProps"), meta.globalProps)
        : meta.globalProps;
    meta.url = url;
    if (source != null) {
      meta.bundle = null;
      meta.byteBuffer = null;
    }
    meta.binaryData = binaryData;
    meta.initialData = initialData;
    meta.globalProps = globalProps;
  }

  // Keeps the original's processor and read-only state for replaced data.
  @Nullable
  private static TemplateData templateData(Object value, @Nullable TemplateData original) {
    if (value == JSONObject.NULL) {
      return null;
    }
    TemplateData data = TemplateData.fromString(value.toString());
    if (original != null) {
      if (original.processorName() != null) {
        data.markState(original.processorName());
      }
      if (original.isReadOnly()) {
        data.markReadOnly();
      }
    }
    return data;
  }

  static final class GlobalEvent {
    final String name;
    final JavaOnlyArray args;
    final boolean dropped;

    GlobalEvent(String name, JavaOnlyArray args, boolean dropped) {
      this.name = name;
      this.args = args;
      this.dropped = dropped;
    }
  }

  @Nullable
  static GlobalEvent beforeSendGlobalEvent(
      LynxTemplateRender render, String name, JavaOnlyArray args) {
    if (!isEnabled()) {
      return null;
    }
    try {
      JSONObject event = new JSONObject();
      event.put("name", name == null ? JSONObject.NULL : name);
      event.put("args", snapshotEventValue(args, new IdentityHashMap<>(), 0));
      byte[] patch = nativeDispatchSendGlobalEvent(
          render.getLogContextSnapshot().viewId, event.toString().getBytes(StandardCharsets.UTF_8));
      return patch == null ? null
                           : applyGlobalEventPatch(name, args,
                               new JSONObject(new String(patch, StandardCharsets.UTF_8)));
    } catch (JSONException | IllegalArgumentException e) {
      LLog.e(TAG, "view.sendGlobalEvent was not intercepted: " + e.getMessage());
      return null;
    }
  }

  private static GlobalEvent applyGlobalEventPatch(
      String name, JavaOnlyArray args, JSONObject patch) throws JSONException {
    if (patch.optBoolean("drop", false)) {
      return new GlobalEvent(name, args, true);
    }
    String nextName = patch.has("name") ? patch.getString("name") : name;
    JavaOnlyArray nextArgs =
        patch.has("args") ? (JavaOnlyArray) decodeEventValue(patch.getJSONArray("args")) : args;
    return new GlobalEvent(nextName, nextArgs, false);
  }

  // Unlike toJSONArray(), reject unsupported/cyclic values instead of silently
  // omitting arguments. The original event remains untouched on any failure.
  private static Object snapshotEventValue(
      Object value, IdentityHashMap<Object, Boolean> parents, int depth) throws JSONException {
    if (depth >= 64) {
      throw new IllegalArgumentException("Event arguments are too deeply nested");
    }
    if (value == null || value == JSONObject.NULL) {
      return JSONObject.NULL;
    }
    if (value instanceof String || value instanceof Boolean) {
      return value;
    }
    if (value instanceof Number) {
      double number = ((Number) value).doubleValue();
      if (Double.isNaN(number) || Double.isInfinite(number)) {
        throw new IllegalArgumentException("Event arguments must contain finite numbers");
      }
      return value;
    }
    if (parents.put(value, true) != null) {
      throw new IllegalArgumentException("Cyclic event arguments");
    }
    try {
      if (value instanceof List) {
        JSONArray array = new JSONArray();
        for (Object item : (List<?>) value) {
          array.put(snapshotEventValue(item, parents, depth + 1));
        }
        return array;
      }
      if (value instanceof Map) {
        JSONObject object = new JSONObject();
        for (Map.Entry<?, ?> entry : ((Map<?, ?>) value).entrySet()) {
          if (!(entry.getKey() instanceof String)) {
            throw new IllegalArgumentException("Event argument keys must be strings");
          }
          object.put(
              (String) entry.getKey(), snapshotEventValue(entry.getValue(), parents, depth + 1));
        }
        return object;
      }
      throw new IllegalArgumentException("Unsupported event argument");
    } finally {
      parents.remove(value);
    }
  }

  private static Object decodeEventValue(Object value) throws JSONException {
    if (value == JSONObject.NULL) {
      return null;
    }
    if (value instanceof JSONArray) {
      JSONArray array = (JSONArray) value;
      JavaOnlyArray result = new JavaOnlyArray();
      for (int i = 0; i < array.length(); ++i) {
        result.add(decodeEventValue(array.get(i)));
      }
      return result;
    }
    if (value instanceof JSONObject) {
      JSONObject object = (JSONObject) value;
      JavaOnlyMap result = new JavaOnlyMap();
      Iterator<String> keys = object.keys();
      while (keys.hasNext()) {
        String key = keys.next();
        result.put(key, decodeEventValue(object.get(key)));
      }
      return result;
    }
    return value;
  }

  @Nullable private static native byte[] nativeDispatchSendGlobalEvent(long viewId, byte[] event);

  private static long nativePtr(@Nullable TemplateData data) {
    if (data == null) {
      return 0;
    }
    data.flush();
    return data.getNativePtr();
  }

  private static native void nativeDispatchCreated(long viewId, String url);

  @Nullable
  private static native byte[] nativeDispatchUpdateMetaData(
      long viewId, long updateData, long globalProps);

  // Returns {patch JSON bytes, replacement template bytes or null}, or null when unchanged.
  @Nullable
  private static native Object[] nativeDispatchLoadTemplate(
      long viewId, byte[] url, int source, int size, long initialData, long globalProps);
}
