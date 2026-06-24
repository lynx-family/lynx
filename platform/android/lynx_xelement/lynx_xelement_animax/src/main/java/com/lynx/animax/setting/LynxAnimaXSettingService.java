// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.animax.setting;

import com.lynx.animax.service.IAnimaXSettingService;
import com.lynx.animax.util.AnimaXLog;
import com.lynx.tasm.service.ILynxTrailService;
import com.lynx.tasm.service.LynxServiceCenter;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Default implementation of {@link IAnimaXSettingService} that retrieves settings
 * through {@link ILynxTrailService}.
 *
 * This service provides access to various AnimaX settings stored in the Trail system.
 * It handles two types of settings:
 * 1. String values
 * 2. Collections of String values
 *
 * The service ensures type safety by properly converting and validating values
 * retrieved from the Trail system.
 *
 * This implementation includes a thread-safe instance-level cache mechanism to significantly
 * improve performance by avoiding repeated Trail service calls for the same keys.
 * The cache ensures fast retrieval speed for frequently accessed configuration values.
 */
public class LynxAnimaXSettingService implements IAnimaXSettingService {
  private static final String TAG = "LynxAnimaXSettingService";

  /**
   * Thread-safe instance-level cache to store retrieved setting values.
   * Uses ConcurrentHashMap to ensure thread safety and fast concurrent access.
   * Once a value is cached, subsequent requests for the same key will return immediately
   * without querying the Trail service, significantly improving performance.
   */
  private final ConcurrentHashMap<String, AnimaXSettingValue> mCache = new ConcurrentHashMap<>();

  /**
   * Clear the instance-level key-value cache.
   */
  public void clearCache() {
    mCache.clear();
  }

  /**
   * Retrieves a setting value for the specified key with caching support.
   *
   * This method first checks the instance-level cache for the requested key to ensure fast
   * retrieval. If the value is cached, it returns immediately without any service calls. If not
   * cached, it fetches the value from the Trail service and stores it in the cache for future fast
   * access, ensuring optimal performance for repeated requests.
   *
   * @param key The key to lookup in the Trail system
   * @return An {@link AnimaXSettingValue} containing either a string or collection value.
   *         Returns an empty value if:
   *         - The Trail service is not available
   *         - The key doesn't exist
   *         - The value type is not supported
   *         - An error occurs during retrieval
   */
  @Override
  public AnimaXSettingValue getValueByKey(String key) {
    if (key == null) {
      return AnimaXSettingValue.empty();
    }

    // Check cache first
    AnimaXSettingValue cachedValue = mCache.get(key);
    if (cachedValue != null) {
      return cachedValue;
    }

    // If not in cache, compute and store atomically
    AnimaXSettingValue newValue = retrieveValueFromService(key);
    AnimaXSettingValue existingValue = mCache.putIfAbsent(key, newValue);

    return existingValue != null ? existingValue : newValue;
  }

  /**
   * Retrieves the actual value from the Trail service.
   * This method contains the original logic for fetching values.
   *
   * @param key The key to lookup in the Trail system
   * @return The retrieved AnimaXSettingValue
   */
  private AnimaXSettingValue retrieveValueFromService(String key) {
    // TODO(zhoupeng): use LynxTrailHub after it is ready
    ILynxTrailService service = LynxServiceCenter.inst().getService(ILynxTrailService.class);
    if (service == null) {
      AnimaXLog.w(TAG, "Trail service not available");
      return AnimaXSettingValue.empty();
    }

    try {
      Object value = service.objectValueForTrailKey(key);
      if (value == null) {
        return AnimaXSettingValue.empty();
      }

      if (value instanceof String) {
        return AnimaXSettingValue.fromString((String) value);
      } else if (value instanceof Collection<?>) {
        return convertToStringCollection((Collection<?>) value);
      } else {
        AnimaXLog.w(TAG, "Unsupported value type: " + value.getClass().getName());
      }
    } catch (Throwable t) {
      AnimaXLog.e(TAG, "Failed to get value for key: " + key + ", message: " + t.getMessage());
    }

    return AnimaXSettingValue.empty();
  }

  /**
   * Safely converts a Collection of unknown type to a Collection of Strings.
   *
   * @param collection The source collection to convert
   * @return An AnimaXSettingValue containing the converted string collection.
   *         Returns an empty collection if the conversion fails or contains non-string elements.
   */
  private AnimaXSettingValue convertToStringCollection(Collection<?> collection) {
    if (collection.isEmpty()) {
      return AnimaXSettingValue.fromCollection(Collections.emptyList());
    }

    ArrayList<String> result = new ArrayList<>(collection.size());
    for (Object item : collection) {
      if (item instanceof String) {
        result.add((String) item);
      } else {
        AnimaXLog.w(TAG,
            "Collection contains non-string element: "
                + (item != null ? item.getClass().getName() : "null"));
        return AnimaXSettingValue.fromCollection(Collections.emptyList());
      }
    }

    return AnimaXSettingValue.fromCollection(result);
  }
}
