// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.group;

import com.lynx.tasm.LynxEngine;
import com.lynx.tasm.TemplateBundle;

/**
 * Interface To Manager Runtime Cache Between LynxViews
 * Shared the same LynxViewGroup;
 */
public interface ILynxViewRuntimeCacheManager {
  /**
   * Set A Generated TemplateBundle to CacheManager
   */
  void setTemplateBundle(TemplateBundle bundle);

  /**
   * Get A Already Generated TemplateBundle from CacheManager
   * @return Already Generated TemplateResult
   */
  TemplateBundle getTemplateBundle();

  /**
   * Cache A Already Loaded LynxEngine to LynxViewGroup
   * @param lynxEngine A used {@link LynxEngine} to be destroyed.
   */
  void setLynxEngine(LynxEngine lynxEngine);

  /**
   * Acquire A Pre Used LynxEngine from LynxViewGroup
   * @return LynxEngine cached previously
   */
  LynxEngine getLynxEngine();

  /**
   * Check if Engine Cache Enabled.
   * @return checks if Engine Cache is enabled.
   */
  boolean isEngineCacheEnabled();

  void setBitmapSizeCache(String source, int width, int height);

  BitmapSize getBitmapSizeCache(String source);
}
