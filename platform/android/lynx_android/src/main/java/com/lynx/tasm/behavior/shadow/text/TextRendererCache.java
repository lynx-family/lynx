// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import android.util.LruCache;
import com.lynx.tasm.behavior.LynxContext;

public class TextRendererCache {
  private LruCache<TextRendererKey, TextRenderer> mCache;
  private static final int MAX_SIZE = 500;

  private static class Holder {
    private static TextRendererCache cache = new TextRendererCache();
  }

  public static TextRendererCache cache() {
    return Holder.cache;
  }

  private TextRendererCache() {
    mCache = new LruCache<>(MAX_SIZE);
  }

  public void onLowMemory() {
    mCache.evictAll();
  }

  public void clearCache() {
    mCache.evictAll();
  }

  public TextRenderer getRenderer(LynxContext context, TextRendererKey key) {
    /// 1. find from global cache
    TextRenderer renderer = mCache.get(key);
    if (renderer != null) {
      return renderer;
    }
    renderer = new TextRenderer(context, key);
    if (renderer.isEnableCache()) {
      mCache.put(key, renderer);
    }
    // TextLayoutWarmer pre-renders a text layout in the background, which helps to cache the glyhps
    // before they are needed on the main thread for display.
    // However, frequent message sending in the main thread itself may introduce some performance
    // issues. We should be cautious when invoking warmLayout, for example, using batchWarmLayout
    // instead of calling it for each text individually. Here we delete this warmLayout logic first
    // and will add it back later once we have figured out the strategy for warmLayout.

    // TextLayoutWarmer.warmer().warmLayout(renderer.getTextLayout());
    return renderer;
  }
}
