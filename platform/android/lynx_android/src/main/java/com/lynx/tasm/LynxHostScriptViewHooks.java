// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import androidx.annotation.Nullable;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.utils.UIThreadUtils;
import java.nio.charset.StandardCharsets;
import java.util.Iterator;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * Debug-engine View registration, Host Script View commands and UI-domain interception entries.
 */
final class LynxHostScriptViewHooks {
  private static final String TAG = "LynxHostScriptViewHooks";
  // Where a load reads its template from; native code names them in events.
  private static final int SOURCE_URL = 0;
  private static final int SOURCE_BYTES = 1;
  private static final int SOURCE_BUFFER = 2;
  private static final int SOURCE_BUNDLE = 3;
  private static volatile boolean sEnabled;
  // Set once by the debug engine; Release engines never register Views.
  private static volatile boolean sRegistryAvailable;

  private LynxHostScriptViewHooks() {}

  @CalledByNative
  private static void setEnabled(boolean enabled) {
    UIThreadUtils.assertOnUiThread();
    sEnabled = enabled;
  }

  @CalledByNative
  private static void setRegistryAvailable() {
    sRegistryAvailable = true;
  }

  static boolean isEnabled() {
    return sEnabled;
  }

  /**
   * Registers an initialized View as operable by Host Script, whether or not DevTool is enabled
   * yet. It stays registered until destroyed and is never retained by the registry.
   */
  static void register(LynxView view, @Nullable LynxTemplateRender render) {
    if (!sRegistryAvailable || render == null) {
      return;
    }
    final long viewId = render.getLogContextSnapshot().viewId;
    if (viewId == LynxLogContext.UNAVAILABLE_ID) {
      return;
    }
    view.addLynxViewClient(new LynxViewClient() {
      @Override
      public void onLoadSuccess() {
        nativeNotifyView(viewId, "loadSuccess", 0, null);
      }

      @Override
      public void onFirstScreen() {
        nativeNotifyView(viewId, "firstScreen", 0, null);
      }

      @Override
      public void onPageUpdate() {
        nativeNotifyView(viewId, "pageUpdate", 0, null);
      }

      @Override
      public void onDataUpdated() {
        nativeNotifyView(viewId, "dataUpdated", 0, null);
      }

      @Override
      public void onUpdateDataWithoutChange() {
        nativeNotifyView(viewId, "dataUpdated", 0, null);
      }

      @Override
      public void onReceivedError(LynxError error) {
        if (error != null) {
          nativeNotifyView(viewId, "error", error.getErrorCode(), error.getSummaryMessage());
        }
      }
    });
    String url = view.getTemplateUrl();
    nativeRegisterView(viewId, view, url == null ? "" : url);
  }

  // Engine replacement also calls LynxViewClient.onDestroy; only the platform View owns this entry.
  static void unregister(@Nullable LynxTemplateRender render) {
    if (!sRegistryAvailable) {
      return;
    }
    long viewId = viewId(render);
    if (viewId != LynxLogContext.UNAVAILABLE_ID) {
      nativeUnregisterView(viewId);
    }
  }

  // Executes one queued command and reports asynchronous platform errors.
  private static void runScriptCommand(long viewId, Runnable command) {
    try {
      command.run();
    } catch (Throwable throwable) {
      nativeNotifyView(viewId, "error", -1, throwable.toString());
    }
  }

  private static long viewId(@Nullable LynxTemplateRender render) {
    return render == null ? LynxLogContext.UNAVAILABLE_ID : render.getLogContextSnapshot().viewId;
  }

  /** Records the URL a load commits, after any interceptor patch. */
  private static void commitUrl(@Nullable LynxTemplateRender render, @Nullable String url) {
    long viewId = viewId(render);
    if (sRegistryAvailable && viewId != LynxLogContext.UNAVAILABLE_ID) {
      nativeUpdateUrl(viewId, url == null ? "" : url);
    }
  }

  /**
   * Dispatches view.loadTemplate before the template is fetched. SSR loads are not covered.
   */
  static void beforeLoadTemplate(LynxTemplateRender render, LynxLoadMeta meta) {
    if (!isEnabled() || meta.loadMode == LynxLoadMode.RENDER_SSR
        || meta.loadMode == LynxLoadMode.HYDRATE_SSR) {
      commitUrl(render, meta.url);
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
    if (patch != null) {
      try {
        applyLoadPatch(meta, new JSONObject(new String((byte[]) patch[0], StandardCharsets.UTF_8)),
            (byte[]) patch[1]);
      } catch (JSONException e) {
        LLog.e(TAG, "view.loadTemplate patch was not applied: " + e.getMessage());
      }
    }
    commitUrl(render, meta.url);
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

  private static long nativePtr(@Nullable TemplateData data) {
    if (data == null) {
      return 0;
    }
    data.flush();
    return data.getNativePtr();
  }

  // Data, props and processor JSON may be null.
  @CalledByNative
  private static void loadTemplate(LynxView view, long viewId, byte[] templateBytes, String url,
      String initialDataJson, String globalPropsJson, String processorName, boolean readOnly) {
    runScriptCommand(viewId, () -> {
      LynxLoadMeta.Builder builder = new LynxLoadMeta.Builder();
      builder.setUrl(url);
      if (templateBytes != null && templateBytes.length > 0) {
        builder.setBinaryData(templateBytes);
      }
      TemplateData initialData = createTemplateData(initialDataJson, processorName, readOnly);
      if (initialData != null) {
        builder.setInitialData(initialData);
      }
      TemplateData globalProps = createTemplateData(globalPropsJson, null, false);
      if (globalProps != null) {
        builder.setGlobalProps(globalProps);
      }
      view.loadTemplate(builder.build());
    });
  }

  @CalledByNative
  private static void loadSSR(LynxView view, long viewId, byte[] ssrBytes, String url,
      String initialDataJson, boolean hydrate) {
    runScriptCommand(viewId, () -> {
      TemplateData initialData = createTemplateData(initialDataJson, null, false);
      if (hydrate) {
        view.ssrHydrate(ssrBytes, url, initialData);
      } else {
        view.renderSSR(ssrBytes, url, initialData);
      }
    });
  }

  @CalledByNative
  private static void updateMetaData(
      LynxView view, long viewId, String dataJson, String globalPropsJson) {
    runScriptCommand(viewId, () -> {
      LynxUpdateMeta.Builder builder = new LynxUpdateMeta.Builder();
      if (dataJson != null) {
        builder.setUpdatedData(createTemplateData(dataJson, null, false));
      }
      if (globalPropsJson != null) {
        builder.setUpdatedGlobalProps(createTemplateData(globalPropsJson, null, false));
      }
      view.updateMetaData(builder.build());
    });
  }

  @CalledByNative
  private static void reloadTemplate(
      LynxView view, long viewId, String dataJson, String globalPropsJson) {
    runScriptCommand(viewId, () -> {
      TemplateData data = createTemplateData(dataJson, null, false);
      TemplateData globalProps = createTemplateData(globalPropsJson, null, false);
      view.reloadTemplate(data == null ? TemplateData.empty() : data, globalProps);
    });
  }

  @CalledByNative
  private static void sendGlobalEvent(LynxView view, long viewId, String name, String paramsJson) {
    runScriptCommand(viewId, () -> {
      JavaOnlyArray params;
      try {
        params = (JavaOnlyArray) eventParam(new JSONArray(paramsJson));
      } catch (JSONException e) {
        throw new IllegalArgumentException(e);
      }
      view.sendGlobalEvent(name, params);
    });
  }

  // Nested containers and JSON null must also use values understood by the bridge.
  @Nullable
  private static Object eventParam(Object value) throws JSONException {
    if (value == JSONObject.NULL) {
      return null;
    }
    if (value instanceof JSONArray) {
      JSONArray array = (JSONArray) value;
      JavaOnlyArray result = new JavaOnlyArray();
      for (int i = 0; i < array.length(); i++) {
        result.add(eventParam(array.get(i)));
      }
      return result;
    }
    if (value instanceof JSONObject) {
      JSONObject object = (JSONObject) value;
      JavaOnlyMap result = new JavaOnlyMap();
      Iterator<String> keys = object.keys();
      while (keys.hasNext()) {
        String key = keys.next();
        result.put(key, eventParam(object.get(key)));
      }
      return result;
    }
    return value;
  }

  @Nullable
  private static TemplateData createTemplateData(
      @Nullable String json, @Nullable String processorName, boolean readOnly) {
    if (json == null || json.isEmpty()) {
      return null;
    }
    TemplateData data = TemplateData.fromString(json);
    if (processorName != null && !processorName.isEmpty()) {
      data.markState(processorName);
    }
    if (readOnly) {
      data.markReadOnly();
    }
    return data;
  }

  private static native void nativeRegisterView(long viewId, LynxView view, String url);
  private static native void nativeUnregisterView(long viewId);
  private static native void nativeUpdateUrl(long viewId, String url);
  private static native void nativeNotifyView(long viewId, String event, int code, String message);

  @Nullable
  private static native byte[] nativeDispatchUpdateMetaData(
      long viewId, long updateData, long globalProps);

  // Returns {patch JSON bytes, replacement template bytes or null}, or null when unchanged.
  @Nullable
  private static native Object[] nativeDispatchLoadTemplate(
      long viewId, byte[] url, int source, int size, long initialData, long globalProps);
}
