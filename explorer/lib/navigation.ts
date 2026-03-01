// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/**
 * Open a URL via the native ExplorerModule.
 */
export function openSchema(url: string): void {
  NativeModules.ExplorerModule.openSchema(url);
}

/**
 * Navigate to a bundle path with optional params. Constructs a
 * file://lynx?local:// URL and opens it via the native ExplorerModule.
 */
export function navigateTo(
  path: string,
  params?: Record<string, string | number>
): void {
  let url = `file://lynx?local://${path}`;
  if (params) {
    const qs = Object.entries(params)
      .map(([k, v]) => `${k}=${v}`)
      .join('&');
    url += `?${qs}`;
  }
  NativeModules.ExplorerModule.openSchema(url);
}

/**
 * Close the current page via the native ExplorerModule.
 */
export function navigateBack(): void {
  NativeModules.ExplorerModule.navigateBack?.();
}
