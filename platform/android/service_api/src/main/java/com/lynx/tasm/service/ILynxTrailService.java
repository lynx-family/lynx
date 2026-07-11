// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.service;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

@Keep
public interface ILynxTrailService extends IServiceProvider {
  /**
   * Get service class, DO NOT OVERRIDE THIS METHOD
   */
  @NonNull
  default Class<? extends IServiceProvider> getServiceClass() {
    return ILynxTrailService.class;
  }

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

  /**
   * Returns an immutable snapshot of the value layers maintained by this service.
   *
   * <p>A layer is a host-defined source of Trail values. Implementations may provide any number of
   * layers with non-empty, unique names. The returned list is ordered from highest to lowest
   * priority: when the same key exists in multiple layers, the value in the first matching layer is
   * effective. The name {@code mock} is reserved for local mock overrides. When present, the mock
   * layer must be the first layer in the list.
   *
   * <p>Each layer contains all values discoverable by the implementation. A layer's {@code
   * updatedAt} is implementation-defined; zero means that its update time is unknown or
   * unsupported.
   */
  @NonNull
  default List<LynxTrailValueLayer> getLayeredValues() {
    return Collections.emptyList();
  }

  /** Sets a process-wide mock override. */
  default boolean setMockValue(@NonNull String key, @NonNull String value) {
    return false;
  }

  /** Removes a process-wide mock override. */
  default boolean removeMockValue(@NonNull String key) {
    return false;
  }

  /** Clears all process-wide mock overrides. */
  default boolean clearMockValues() {
    return false;
  }

  /** Fetches the latest values from the implementation-defined backing source. */
  default void fetchLatestSettings(@NonNull FetchCallback callback) {
    callback.onResult(false, "Latest settings fetch is not supported");
  }

  interface FetchCallback {
    void onResult(boolean success, @Nullable String errorMessage);
  }

  /** Immutable values and metadata for one host-defined Trail layer. */
  final class LynxTrailValueLayer {
    @NonNull private final String name;
    private final long updatedAt;
    @NonNull private final Map<String, String> values;

    public LynxTrailValueLayer(
        @NonNull String name, long updatedAt, @NonNull Map<String, String> values) {
      this.name = name;
      this.updatedAt = updatedAt;
      this.values = Collections.unmodifiableMap(new LinkedHashMap<>(values));
    }

    @NonNull
    public String getName() {
      return name;
    }

    public long getUpdatedAt() {
      return updatedAt;
    }

    @NonNull
    public Map<String, String> getValues() {
      return values;
    }
  }
}
