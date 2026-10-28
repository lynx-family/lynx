// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import type { NativeApp } from './interface';

type Entry = { creator: number; flags: number };
// 0 = BaseApp._$factoryCache; 1 = BaseApp._$loadScriptCache.
type CacheType = 0 | 1;

/** Scalar sidecar for the two global caches; never retains an app or exports. */
export class ModuleCacheMonitor {
  // Admit the first 128 creators across both caches for this corejs realm.
  private creators = new Set<number>();
  private entries: [Map<string, Entry>, Map<string, Entry>] = [
    new Map(),
    new Map(),
  ];

  insert(type: CacheType, key: string, nativeApp: NativeApp) {
    const creator = Number(nativeApp.id);
    if (!this.creators.has(creator)) {
      if (this.creators.size >= 128) {
        // An untracked replacement must not inherit the old entry's creator.
        this.remove(type, key);
        return;
      }
      this.creators.add(creator);
    }
    this.entries[type].set(key, { creator, flags: 0 });
    nativeApp.reportModuleCache(type, creator, 1, 0, 0);
  }

  hit(type: CacheType, key: string, nativeApp: NativeApp) {
    const entry = this.entries[type].get(key);
    if (!entry) return;
    const reused = !(entry.flags & 1);
    const cross = !(entry.flags & 2) && entry.creator !== Number(nativeApp.id);
    if (!reused && !cross) return;
    entry.flags |= cross ? 3 : 1;
    nativeApp.reportModuleCache(type, entry.creator, 0, +reused, +cross);
  }

  remove(type: CacheType, key: string) {
    this.entries[type].delete(key);
  }
}
