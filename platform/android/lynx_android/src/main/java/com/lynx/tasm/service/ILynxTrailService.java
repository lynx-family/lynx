// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.service;

import android.content.Context;
import androidx.annotation.NonNull;
import java.util.Map;

public interface ILynxTrailService extends IServiceProvider {
  /**
   * Get service class, DO NOT OVERRIDE THIS METHOD
   */
  @NonNull
  default Class<? extends IServiceProvider> getServiceClass() {
    return ILynxTrailService.class;
  }

  /**
   * Initialize service with context
   * @param context
   */
  void initialize(Context context);

  /**
   * Get string value for key from experiment
   * @param key
   * @return value string
   */
  String stringValueForTrailKey(@NonNull String key);

  /**
   * Get object value for key from experiment.Only used for compatibility with different types,
   * please use stringValueForTrailKey in most cases
   * @param key
   * @return value object
   */
  Object objectValueForTrailKey(@NonNull String key);

  /**
   * Get all values for key from experiment.
   */
  Map<String, Object> getAllValues();
}
