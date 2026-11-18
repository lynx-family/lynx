// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.recorder;

import android.util.Base64;
import androidx.annotation.Nullable;
import com.lynx.tasm.TemplateBundle;
import com.lynx.tasm.TemplateBundleOption;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.resourceprovider.LynxResourceCallback;
import com.lynx.tasm.resourceprovider.LynxResourceRequest;
import com.lynx.tasm.resourceprovider.LynxResourceResponse;
import com.lynx.tasm.resourceprovider.template.LynxTemplateResourceFetcher;
import com.lynx.tasm.resourceprovider.template.TemplateProviderResult;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.Map;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/** Supplies lazy bundles decoded before an opted-in replay. */
final class LynxRecorderTemplateFetcher extends LynxTemplateResourceFetcher {
  private final LynxRecorderFetcher mRecordedFetcher;
  private final Map<String, TemplateBundle> mPreparedBundles;

  LynxRecorderTemplateFetcher(
      LynxRecorderFetcher recordedFetcher, Map<String, TemplateBundle> preparedBundles) {
    mRecordedFetcher = recordedFetcher;
    mPreparedBundles = preparedBundles;
  }

  @Nullable
  static Map<String, TemplateBundle> prepare(
      @Nullable JSONObject config, @Nullable JSONArray actions) throws JSONException {
    JSONObject settings = config != null ? config.optJSONObject("replayTemplateCache") : null;
    if (settings == null) {
      return null;
    }
    if (!settings.getBoolean("predecode")) {
      return Collections.emptyMap();
    }

    JSONArray urls = settings.getJSONArray("urls");
    if (urls.length() == 0 || actions == null) {
      throw new JSONException("Missing replay template cache inputs");
    }
    Map<String, JSONObject> sources = new LinkedHashMap<>();
    for (int i = 0; i < urls.length(); ++i) {
      String url = urls.getString(i);
      if (url.isEmpty() || sources.containsKey(url)) {
        throw new JSONException("Invalid replay template cache URL: " + url);
      }
      sources.put(url, null);
    }
    for (int i = 0; i < actions.length(); ++i) {
      JSONObject action = actions.optJSONObject(i);
      if (action == null
          || !"LoadComponentWithCallback".equals(action.optString("Function Name"))) {
        continue;
      }
      JSONObject params = action.getJSONObject("Params");
      String url = params.optString("url");
      if (sources.containsKey(url)) {
        sources.put(url, params);
      }
    }
    Map<String, TemplateBundle> bundles = new LinkedHashMap<>();
    TraceEvent.beginSection("TestBench.PrepareTemplateBundles");
    try {
      for (Map.Entry<String, JSONObject> source : sources.entrySet()) {
        String url = source.getKey();
        JSONObject params = source.getValue();
        if (params == null || !params.optBoolean("sync_tag")) {
          throw new JSONException("Expected a recorded template using a direct callback: " + url);
        }
        TraceEvent.beginSection("TestBench.PredecodeTemplateBundle");
        try {
          byte[] bytes = Base64.decode(params.getString("source"), Base64.DEFAULT);
          TemplateBundleOption options = new TemplateBundleOption.Builder().setUrl(url).build();
          TemplateBundle bundle = TemplateBundle.fromTemplate(bytes, options);
          if (bundle == null || !bundle.isValid()) {
            throw new JSONException("Unable to predecode replay template: " + url);
          }
          bundles.put(url, bundle);
        } finally {
          TraceEvent.endSection("TestBench.PredecodeTemplateBundle");
        }
      }
    } finally {
      TraceEvent.endSection("TestBench.PrepareTemplateBundles");
    }
    return bundles;
  }

  @Override
  @SuppressWarnings("unchecked")
  public void fetchTemplate(
      LynxResourceRequest request, LynxResourceCallback<TemplateProviderResult> callback) {
    if (request.getResourceType()
        != LynxResourceRequest.LynxResourceType.LynxResourceTypeDynamicComponent) {
      callback.onResponse(LynxResourceResponse.onFailed(
          new UnsupportedOperationException("Replay template cache only serves lazy bundles")));
      return;
    }
    String url = request.getUrl();
    TemplateBundle bundle = mPreparedBundles.get(url);
    if (bundle != null) {
      TraceEvent.beginSection("TestBench.ReplayTemplateCacheHit");
      try {
        callback.onResponse(
            LynxResourceResponse.onSuccess(TemplateProviderResult.fromTemplateBundle(bundle)));
      } finally {
        TraceEvent.endSection("TestBench.ReplayTemplateCacheHit");
      }
      return;
    }
    mRecordedFetcher.loadDynamicComponent(url, (bytes, error) -> {
      if (error == null && bytes != null) {
        callback.onResponse(
            LynxResourceResponse.onSuccess(TemplateProviderResult.fromBinary(bytes)));
      } else {
        callback.onResponse(LynxResourceResponse.onFailed(error != null
                ? error
                : new IllegalStateException("Missing recorded lazy bundle: " + url)));
      }
    });
  }

  @Override
  @SuppressWarnings("unchecked")
  public void fetchSSRData(LynxResourceRequest request, LynxResourceCallback<byte[]> callback) {
    callback.onResponse(LynxResourceResponse.onFailed(
        new UnsupportedOperationException("Replay template cache does not serve SSR data")));
  }
}
