// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.service;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.LynxView;
import org.json.JSONObject;

public interface ILynxMonitorService extends IServiceProvider {
  /**
   * Get service class, DO NOT OVERRIDE THIS METHOD
   */
  @NonNull
  default Class<? extends IServiceProvider> getServiceClass() {
    return ILynxMonitorService.class;
  }

  /**
   * report data about loading LynxView
   * @param eventName
   * @param data
   */
  void reportTrailEvent(@NonNull String eventName, @NonNull JSONObject data);

  /**
   * report data about loading image
   * @param eventName
   * @param data
   */
  void reportImageStatus(@NonNull String eventName, @NonNull JSONObject data);

  /**
   * custom report data
   * @param view
   * @param eventName
   * @param data
   * @param metrics
   * @param category
   */
  void formatEventReporter(@Nullable LynxView view, @NonNull String eventName,
      @NonNull JSONObject data, @Nullable JSONObject metrics, @Nullable JSONObject category);

  /**
   * report data about loading resource
   * @param view
   * @param eventName
   * @param data
   * @param extra
   */
  void reportResourceStatus(@NonNull LynxView view, @NonNull String eventName,
      @NonNull JSONObject data, @Nullable JSONObject extra);

  /**
   * report tag in global context when app crash
   * @param tagName
   * @param tagValue
   */
  void reportCrashGlobalContextTag(@NonNull String tagName, @NonNull String tagValue);

  /**
   * report image info
   * TODO(zhoupeng): standardize the interface of ILynxMonitorService
   * @param imageInfo
   */
  void reportImageInfo(@NonNull LynxImageInfo imageInfo);
}
