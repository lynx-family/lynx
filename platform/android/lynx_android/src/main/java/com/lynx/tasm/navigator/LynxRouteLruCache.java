// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.navigator;

import android.util.LruCache;
import com.lynx.tasm.LynxView;

public class LynxRouteLruCache extends LruCache<LynxRoute, LynxView> {
  private LynxRouteCacheListener listener;

  /**
   * @param maxSize for caches that do not override {@link #sizeOf}, this is
   *                the maximum number of entries in the cache. For all other caches,
   *                this is the maximum sum of the sizes of the entries in this cache.
   */
  public LynxRouteLruCache(int maxSize, LynxRouteCacheListener listener) {
    super(maxSize);
    this.listener = listener;
  }

  @Override
  protected LynxView create(LynxRoute key) {
    return null;
  }

  protected final void get(LynxRoute route, LynxViewCreationListener listener) {
    LynxView view = get(route);
    if (view != null) {
      listener.onReady(view);
    } else {
      this.listener.onLynxViewRecreated(route, listener);
    }
  }

  @Override
  protected void entryRemoved(
      boolean evicted, LynxRoute key, LynxView oldValue, LynxView newValue) {
    if (evicted && this.listener != null) {
      this.listener.onLynxViewEvicted(oldValue);
    }
  }

  public interface LynxRouteCacheListener {
    void onLynxViewEvicted(LynxView view);

    void onLynxViewRecreated(LynxRoute key, LynxViewCreationListener listener);
  }
}
