// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

const STORAGE_KEY = 'explorer_recent_urls';
const MAX_ITEMS = 20;

let memoryCache: string[] | null = null;

function readStorage(): string[] {
  if (memoryCache !== null) {
    return memoryCache;
  }
  try {
    const data = (lynx as any).getStorageSync(STORAGE_KEY);
    if (typeof data === 'string') {
      const parsed = JSON.parse(data);
      if (Array.isArray(parsed)) {
        memoryCache = parsed;
        return parsed;
      }
    }
  } catch {
    // Storage API not available
  }
  memoryCache = [];
  return [];
}

function writeStorage(urls: string[]): void {
  memoryCache = urls;
  try {
    (lynx as any).setStorageSync(STORAGE_KEY, JSON.stringify(urls));
  } catch {
    // Persist in memory only
  }
}

export function getRecentUrls(): string[] {
  return readStorage();
}

export function addRecentUrl(url: string): string[] {
  const current = readStorage();
  const filtered = current.filter((u) => u !== url);
  filtered.unshift(url);
  if (filtered.length > MAX_ITEMS) {
    filtered.length = MAX_ITEMS;
  }
  writeStorage(filtered);
  return filtered;
}

export function clearRecentUrls(): void {
  writeStorage([]);
}
