// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;

import android.app.Application;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.service.ILynxTrailService;
import com.lynx.tasm.service.ILynxTrailService.LynxTrailValueLayer;
import com.lynx.tasm.service.LynxServiceCenter;
import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import org.json.JSONObject;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxSettingPlatformAndroidTest {
  private FakeTrailService mService;

  @Before
  public void setUp() {
    Application application = (Application) InstrumentationRegistry.getInstrumentation()
                                  .getTargetContext()
                                  .getApplicationContext();
    LynxServiceCenter.inst().initialize(application);
    mService = new FakeTrailService();
    LynxServiceCenter.inst().registerService(mService);
  }

  @After
  public void tearDown() {
    LynxServiceCenter.inst().unregisterService(ILynxTrailService.class);
  }

  @Test
  public void handleReturnsValuesLayersAndSources() throws Exception {
    Map<String, String> mockValues = new LinkedHashMap<>();
    mockValues.put("shared", "mock");
    mockValues.put("mockOnly", "mockValue");
    Map<String, String> settingsValues = new LinkedHashMap<>();
    settingsValues.put("shared", "settings");
    settingsValues.put("settingsOnly", "settingsValue");
    mService.mLayers.add(new LynxTrailValueLayer("mock", 30, mockValues));
    mService.mLayers.add(new LynxTrailValueLayer("settings", 20, settingsValues));

    Result values = call("LynxSetting.getValues", null, null);
    assertNull(values.mError);
    JSONObject mergedValues = new JSONObject(values.mJson).getJSONObject("values");
    assertEquals("mock", mergedValues.getString("shared"));
    assertEquals("settingsValue", mergedValues.getString("settingsOnly"));

    Result layered = call("LynxSetting.getLayeredValues", null, null);
    JSONObject layeredJson = new JSONObject(layered.mJson);
    assertEquals(2, layeredJson.getJSONArray("layers").length());
    assertEquals("mock", layeredJson.getJSONObject("merged").getString("shared"));

    Result value = call("LynxSetting.getValue", "shared", null);
    JSONObject valueJson = new JSONObject(value.mJson);
    assertEquals("mock", valueJson.getString("source"));
    assertEquals("mock", valueJson.getString("value"));
    assertEquals(2, valueJson.getJSONArray("layers").length());

    Result missingValue = call("LynxSetting.getValue", "missing", null);
    JSONObject missingValueJson = new JSONObject(missingValue.mJson);
    assertEquals("none", missingValueJson.getString("source"));
    assertEquals(JSONObject.NULL, missingValueJson.get("value"));

    Result fetchInfo = call("LynxSetting.getFetchInfo", null, null);
    assertEquals(20, new JSONObject(fetchInfo.mJson).getLong("settingsTime"));
  }

  @Test
  public void handleCompletesMockMutations() {
    mService.mSetMockValueResult = true;
    mService.mRemoveMockValueResult = false;
    mService.mClearMockValuesResult = true;

    assertNull(call("LynxSetting.setMockValue", "key", "value").mError);
    assertEquals(
        "Failed to persist mock value", call("LynxSetting.removeMockValue", "key", null).mError);
    assertNull(call("LynxSetting.clearMockValues", null, null).mError);
  }

  @Test
  public void handleCompletesFetchAndRejectsUnknownMethods() throws Exception {
    Result success = call("LynxSetting.fetchLatest", null, null);
    assertNull(success.mError);
    assertEquals(0, new JSONObject(success.mJson).getJSONObject("values").length());

    mService.mFetchSuccess = false;
    Result defaultFailure = call("LynxSetting.fetchLatest", null, null);
    assertEquals("Fetch latest settings failed", defaultFailure.mError);

    mService.mFetchError = "network error";
    Result explicitFailure = call("LynxSetting.fetchLatest", null, null);
    assertEquals("network error", explicitFailure.mError);

    Result unknown = call("LynxSetting.unknown", null, null);
    assertEquals("Not implemented: LynxSetting.unknown", unknown.mError);
  }

  @Test
  public void handleReportsMissingService() {
    LynxServiceCenter.inst().unregisterService(ILynxTrailService.class);

    Result result = call("LynxSetting.getValues", null, null);

    assertEquals("{}", result.mJson);
    assertEquals("Lynx Trail Service not registered", result.mError);
  }

  private Result call(String method, String key, String value) {
    Result result = new Result();
    LynxSettingPlatformAndroid.handle(method, key, value, (json, error) -> {
      result.mJson = json;
      result.mError = error;
    });
    return result;
  }

  private static final class Result {
    private String mJson;
    private String mError;
  }

  private static final class FakeTrailService implements ILynxTrailService {
    private final List<LynxTrailValueLayer> mLayers = new ArrayList<>();
    private boolean mSetMockValueResult;
    private boolean mRemoveMockValueResult;
    private boolean mClearMockValuesResult;
    private boolean mFetchSuccess = true;
    private String mFetchError;

    @Override
    public String stringValueForTrailKey(String key) {
      return null;
    }

    @Override
    public Object objectValueForTrailKey(String key) {
      return null;
    }

    @Override
    public Map<String, Object> getAllValues() {
      return Collections.emptyMap();
    }

    @Override
    public List<LynxTrailValueLayer> getLayeredValues() {
      return mLayers;
    }

    @Override
    public boolean setMockValue(String key, String value) {
      return mSetMockValueResult;
    }

    @Override
    public boolean removeMockValue(String key) {
      return mRemoveMockValueResult;
    }

    @Override
    public boolean clearMockValues() {
      return mClearMockValuesResult;
    }

    @Override
    public void fetchLatestSettings(FetchCallback callback) {
      callback.onResult(mFetchSuccess, mFetchError);
    }
  }
}
