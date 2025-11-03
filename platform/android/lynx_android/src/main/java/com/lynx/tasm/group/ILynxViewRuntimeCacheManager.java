// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.group;

import com.lynx.tasm.ILynxEngine;
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

  void setLynxEngine(ILynxEngine lynxEngine);

  ILynxEngine getLynxEngine();

  void setBitmapSizeCache(String source, int width, int height);

  BitmapSize getBitmapSizeCache(String source);
}
