// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import {
  navigate as sparklingNavigate,
  open as sparklingOpen,
  close as sparklingClose,
} from 'sparkling-navigation';
import { addRecentUrl } from './recentHistory';

function getGlobalProps(): Record<string, unknown> | undefined {
  try {
    return lynx.__globalProps as Record<string, unknown> | undefined;
  } catch {
    return undefined;
  }
}

function hasSparklingPipe(): boolean {
  try {
    if (typeof NativeModules === 'undefined') {
      return false;
    }
    const modules = NativeModules as Record<string, unknown>;
    const spkPipe = modules.spkPipe as { call?: unknown } | undefined;
    return typeof spkPipe?.call === 'function';
  } catch {
    return false;
  }
}

/**
 * Returns true if the Sparkling SDK is loaded and the spkPipe module is
 * registered. Used for informational display (e.g. Settings page).
 */
export function isSparklingAvailable(): boolean {
  const props = getGlobalProps();
  const hasNativeSignal =
    props?.sparklingAvailable === true ||
    props?.sparklingNavigation === true ||
    (typeof props?.containerID === 'string' && props.containerID.length > 0);
  return hasNativeSignal && hasSparklingPipe();
}

/**
 * Returns true when running inside a full Sparkling container that supports
 * pipe-based navigation. LynxExplorer uses legacy ExplorerModule navigation,
 * so this returns false unless the native side explicitly sets the flag.
 */
export function isSparkling(): boolean {
  const props = getGlobalProps();
  return isSparklingAvailable() && props?.sparklingNavigation === true;
}

const LOCAL_PREFIX = 'file://lynx?local://';

/**
 * Convert a legacy file://lynx?local:// URL to a hybrid:// scheme.
 * http/https URLs are passed through unchanged.
 */
function toHybridScheme(url: string): string {
  if (url.startsWith(LOCAL_PREFIX)) {
    const remainder = url.substring(LOCAL_PREFIX.length);
    const qIdx = remainder.indexOf('?');
    if (qIdx !== -1) {
      const bundle = remainder.substring(0, qIdx);
      const query = remainder.substring(qIdx + 1);
      return `hybrid://lynxview_page?bundle=${bundle}&${query}`;
    }
    return `hybrid://lynxview_page?bundle=${remainder}`;
  }
  return url;
}

/**
 * Open a URL. When running in a Sparkling container, converts to hybrid://
 * and uses sparkling-navigation. Otherwise falls back to
 * NativeModules.ExplorerModule.openSchema().
 */
export function openSchema(url: string): void {
  addRecentUrl(url);
  if (isSparkling()) {
    const scheme = toHybridScheme(url);
    sparklingOpen({ scheme }, (res) => {
      if (res.code !== 1) {
        console.warn(
          '[navigation] sparkling open failed, falling back to legacy:',
          res.msg
        );
        NativeModules.ExplorerModule.openSchema(url);
      }
    });
  } else {
    NativeModules.ExplorerModule.openSchema(url);
  }
}

/**
 * Navigate to a bundle path with optional params. Uses sparkling-navigation's
 * navigate() inside a Sparkling container, falls back to openSchema with
 * legacy URL format otherwise.
 */
export function navigateTo(
  path: string,
  params?: Record<string, string | number>
): void {
  const legacyOpen = () => {
    let url = `file://lynx?local://${path}`;
    if (params) {
      const qs = Object.entries(params)
        .map(([k, v]) => `${k}=${v}`)
        .join('&');
      url += `?${qs}`;
    }
    NativeModules.ExplorerModule.openSchema(url);
  };

  if (isSparkling()) {
    sparklingNavigate(
      {
        path,
        options: params ? { params } : undefined,
      },
      (res) => {
        if (res.code !== 1) {
          console.warn(
            '[navigation] sparkling navigate failed, falling back to legacy:',
            res.msg
          );
          legacyOpen();
        }
      }
    );
  } else {
    legacyOpen();
  }
}

/**
 * Close the current page. In a Sparkling container uses sparkling-navigation's
 * close(). Otherwise uses NativeModules.ExplorerModule.navigateBack().
 */
export function navigateBack(): void {
  if (isSparkling()) {
    sparklingClose();
  } else {
    NativeModules.ExplorerModule.navigateBack?.();
  }
}
