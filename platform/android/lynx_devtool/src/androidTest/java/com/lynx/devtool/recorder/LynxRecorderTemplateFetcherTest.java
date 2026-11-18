// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.recorder;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.util.Base64;
import com.lynx.tasm.TemplateBundle;
import com.lynx.tasm.resourceprovider.LynxResourceRequest;
import com.lynx.tasm.resourceprovider.LynxResourceRequest.LynxResourceType;
import com.lynx.tasm.resourceprovider.LynxResourceResponse;
import com.lynx.tasm.resourceprovider.LynxResourceResponse.ResponseState;
import com.lynx.tasm.resourceprovider.template.TemplateProviderResult;
import java.util.Collections;
import java.util.Map;
import java.util.concurrent.atomic.AtomicReference;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.junit.Test;

public class LynxRecorderTemplateFetcherTest {
  private static final String URL = "https://example.com/lazy.bundle";
  private static final byte[] RECORDED_BYTES = {1, 2, 3};

  @Test
  public void noCacheConfigLeavesReplayOnItsDefaultPath() throws JSONException {
    assertNull(LynxRecorderTemplateFetcher.prepare(null, null));
    assertNull(LynxRecorderTemplateFetcher.prepare(new JSONObject(), null));
  }

  @Test
  public void disabledPredecodeServesRecordedBytes() throws JSONException {
    Map<String, TemplateBundle> bundles =
        LynxRecorderTemplateFetcher.prepare(cacheConfig(false), null);
    assertTrue(bundles.isEmpty());

    LynxRecorderTemplateFetcher fetcher =
        new LynxRecorderTemplateFetcher(recordedFetcher(), bundles);
    assertRecordedBytes(fetchLazyBundle(fetcher, URL));
  }

  @Test
  public void servesPreparedBundleForExactUrlAndFallsBackToRecordedBytes() throws JSONException {
    TemplateBundle prepared = new TemplateBundle();
    LynxRecorderTemplateFetcher fetcher =
        new LynxRecorderTemplateFetcher(recordedFetcher(), Collections.singletonMap(URL, prepared));

    LynxResourceResponse<TemplateProviderResult> hit = fetchLazyBundle(fetcher, URL);
    assertEquals(ResponseState.SUCCESS, hit.getState());
    assertSame(prepared, hit.getData().getTemplateBundle());
    assertNull(hit.getData().getTemplateBinary());
    assertRecordedBytes(fetchLazyBundle(fetcher, "/lazy.bundle"));
  }

  @Test
  public void rejectsMissingOrDuplicatePredecodeUrls() throws JSONException {
    assertPrepareFails(cacheConfig(true), recordedActions(true), "Missing replay template cache");
    assertPrepareFails(
        cacheConfig(true, URL, URL), recordedActions(true), "Invalid replay template cache URL");
  }

  @Test
  public void predecodeRequiresExactUrlAndDirectCallback() throws JSONException {
    assertPrepareFails(
        cacheConfig(true, "/lazy.bundle"), recordedActions(true), "using a direct callback");
    assertPrepareFails(cacheConfig(true, URL), recordedActions(false), "using a direct callback");
  }

  @Test
  public void propagatesRecordedFetcherError() {
    Exception error = new IllegalStateException("recorded fetch failed");
    LynxRecorderFetcher recorded = new LynxRecorderFetcher() {
      @Override
      public void loadDynamicComponent(String url, LoadedHandler handler) {
        handler.onComponentLoaded(null, error);
      }
    };
    LynxRecorderTemplateFetcher fetcher =
        new LynxRecorderTemplateFetcher(recorded, Collections.emptyMap());

    LynxResourceResponse<TemplateProviderResult> response = fetchLazyBundle(fetcher, URL);
    assertEquals(ResponseState.FAILED, response.getState());
    assertSame(error, response.getError());
  }

  @Test
  public void rejectsUnsupportedRequests() {
    LynxRecorderTemplateFetcher fetcher =
        new LynxRecorderTemplateFetcher(new LynxRecorderFetcher(), Collections.emptyMap());
    LynxResourceRequest request =
        new LynxResourceRequest(URL, LynxResourceType.LynxResourceTypeTemplate);
    AtomicReference<LynxResourceResponse<TemplateProviderResult>> template =
        new AtomicReference<>();
    AtomicReference<LynxResourceResponse<byte[]>> ssr = new AtomicReference<>();

    fetcher.fetchTemplate(request, template ::set);
    fetcher.fetchSSRData(request, ssr::set);
    assertEquals(ResponseState.FAILED, template.get().getState());
    assertTrue(template.get().getError() instanceof UnsupportedOperationException);
    assertEquals(ResponseState.FAILED, ssr.get().getState());
    assertTrue(ssr.get().getError() instanceof UnsupportedOperationException);
  }

  private static JSONObject cacheConfig(boolean predecode, String... urls) throws JSONException {
    JSONArray selectedUrls = new JSONArray();
    for (String url : urls) {
      selectedUrls.put(url);
    }
    return new JSONObject().put("replayTemplateCache",
        new JSONObject().put("predecode", predecode).put("urls", selectedUrls));
  }

  private static JSONArray recordedActions(boolean sync) throws JSONException {
    JSONObject params = new JSONObject()
                            .put("url", URL)
                            .put("source", Base64.encodeToString(RECORDED_BYTES, Base64.NO_WRAP))
                            .put("sync_tag", sync);
    return new JSONArray().put(
        new JSONObject().put("Function Name", "LoadComponentWithCallback").put("Params", params));
  }

  private static LynxRecorderFetcher recordedFetcher() throws JSONException {
    LynxRecorderFetcher recorded = new LynxRecorderFetcher();
    recorded.parse(recordedActions(true));
    return recorded;
  }

  private static LynxResourceResponse<TemplateProviderResult> fetchLazyBundle(
      LynxRecorderTemplateFetcher fetcher, String url) {
    AtomicReference<LynxResourceResponse<TemplateProviderResult>> response =
        new AtomicReference<>();
    fetcher.fetchTemplate(
        new LynxResourceRequest(url, LynxResourceType.LynxResourceTypeDynamicComponent),
        response::set);
    return response.get();
  }

  private static void assertRecordedBytes(LynxResourceResponse<TemplateProviderResult> response) {
    assertEquals(ResponseState.SUCCESS, response.getState());
    assertArrayEquals(RECORDED_BYTES, response.getData().getTemplateBinary());
    assertNull(response.getData().getTemplateBundle());
  }

  private static void assertPrepareFails(JSONObject config, JSONArray actions, String message) {
    try {
      LynxRecorderTemplateFetcher.prepare(config, actions);
      fail("Expected invalid predecode configuration");
    } catch (JSONException e) {
      assertTrue(e.getMessage(), e.getMessage().contains(message));
    }
  }
}
