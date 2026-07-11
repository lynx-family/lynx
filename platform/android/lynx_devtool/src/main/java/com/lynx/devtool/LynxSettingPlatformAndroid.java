// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool;

import com.lynx.tasm.service.ILynxTrailService;
import com.lynx.tasm.service.ILynxTrailService.LynxTrailValueLayer;
import com.lynx.tasm.service.LynxServiceCenter;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

final class LynxSettingPlatformAndroid {
  interface Callback {
    void onResult(String resultJson, String errorMessage);
  }

  private LynxSettingPlatformAndroid() {}

  static void handle(String method, String key, String value, Callback callback) {
    ILynxTrailService service = LynxServiceCenter.inst().getService(ILynxTrailService.class);
    if (service == null) {
      callback.onResult("{}", "Lynx Trail Service not registered");
      return;
    }
    try {
      switch (method) {
        case "LynxSetting.getValues":
          callback.onResult(valuesResult(service), null);
          return;
        case "LynxSetting.getLayeredValues":
          callback.onResult(layeredResult(service), null);
          return;
        case "LynxSetting.getValue":
          callback.onResult(valueResult(service, key), null);
          return;
        case "LynxSetting.setMockValue":
          completeMutation(service.setMockValue(key, value), callback);
          return;
        case "LynxSetting.removeMockValue":
          completeMutation(service.removeMockValue(key), callback);
          return;
        case "LynxSetting.clearMockValues":
          completeMutation(service.clearMockValues(), callback);
          return;
        case "LynxSetting.getFetchInfo":
          callback.onResult(fetchInfoResult(service), null);
          return;
        case "LynxSetting.fetchLatest":
          service.fetchLatestSettings((success, errorMessage) -> {
            if (!success) {
              callback.onResult("{}",
                  errorMessage == null || errorMessage.isEmpty() ? "Fetch latest settings failed"
                                                                 : errorMessage);
              return;
            }
            try {
              callback.onResult(valuesResult(service), null);
            } catch (JSONException exception) {
              callback.onResult("{}", "Failed to serialize LynxSetting result");
            }
          });
          return;
        default:
          callback.onResult("{}", "Not implemented: " + method);
      }
    } catch (JSONException exception) {
      callback.onResult("{}", "Failed to serialize LynxSetting result");
    }
  }

  private static void completeMutation(boolean success, Callback callback) {
    callback.onResult("{}", success ? null : "Failed to persist mock value");
  }

  private static String valuesResult(ILynxTrailService service) throws JSONException {
    JSONObject result = new JSONObject();
    result.put("values", new JSONObject(mergedValues(service.getLayeredValues())));
    return result.toString();
  }

  private static String layeredResult(ILynxTrailService service) throws JSONException {
    List<LynxTrailValueLayer> layers = service.getLayeredValues();
    JSONObject result = new JSONObject();
    result.put("layers", layersJson(layers));
    result.put("merged", new JSONObject(mergedValues(layers)));
    return result.toString();
  }

  private static String valueResult(ILynxTrailService service, String key) throws JSONException {
    List<LynxTrailValueLayer> layers = service.getLayeredValues();
    JSONArray layerResults = new JSONArray();
    String source = "none";
    String value = null;
    for (LynxTrailValueLayer layer : layers) {
      String layerValue = layer.getValues().get(key);
      JSONObject layerJson = new JSONObject();
      layerJson.put("name", layer.getName());
      layerJson.put("updatedAt", layer.getUpdatedAt());
      layerJson.put(
          "values", layerValue == null ? new JSONObject() : new JSONObject().put(key, layerValue));
      layerResults.put(layerJson);
      if (value == null && layerValue != null) {
        source = layer.getName();
        value = layerValue;
      }
    }
    JSONObject result = new JSONObject();
    result.put("key", key);
    result.put("source", source);
    result.put("value", value == null ? JSONObject.NULL : value);
    result.put("layers", layerResults);
    return result.toString();
  }

  private static String fetchInfoResult(ILynxTrailService service) throws JSONException {
    long settingsTime = 0;
    for (LynxTrailValueLayer layer : service.getLayeredValues()) {
      if (!"mock".equals(layer.getName())) {
        settingsTime = Math.max(settingsTime, layer.getUpdatedAt());
      }
    }
    return new JSONObject().put("settingsTime", settingsTime).toString();
  }

  private static JSONArray layersJson(List<LynxTrailValueLayer> layers) throws JSONException {
    JSONArray result = new JSONArray();
    for (LynxTrailValueLayer layer : layers) {
      JSONObject layerJson = new JSONObject();
      layerJson.put("name", layer.getName());
      layerJson.put("updatedAt", layer.getUpdatedAt());
      layerJson.put("values", new JSONObject(layer.getValues()));
      result.put(layerJson);
    }
    return result;
  }

  private static Map<String, String> mergedValues(List<LynxTrailValueLayer> layers) {
    Map<String, String> result = new LinkedHashMap<>();
    for (LynxTrailValueLayer layer : layers) {
      for (Map.Entry<String, String> entry : layer.getValues().entrySet()) {
        if (!result.containsKey(entry.getKey())) {
          result.put(entry.getKey(), entry.getValue());
        }
      }
    }
    return result;
  }
}
