// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import {
  navigate as sparklingNavigate,
  open as sparklingOpen,
  close as sparklingClose,
} from 'sparkling-navigation';
import { addRecentSession } from './recentHistory';
import type { LaunchSessionSource } from './recentHistory';
import { runtimeRequiredByURL } from './launchCommand';
import type { ExplorerRuntime } from './launchCommand';

export type ContainerRequest = 'automatic' | 'legacy' | 'sparkling';

interface NativeRouteResult {
  success: boolean;
  code: string;
  message: string;
}

type NativeRouteCallback = (result: NativeRouteResult) => void;

interface ExplorerNativeModule {
  openSchema(url: string): void;
  openRoute?(
    url: string,
    container: ContainerRequest,
    callback: NativeRouteCallback
  ): void;
  navigateBack?(callback: NativeRouteCallback): void;
}

interface SparklingRouteResult {
  code: number;
  msg: string;
}

function getGlobalProps(): Record<string, unknown> | undefined {
  try {
    return (lynx.__globalProps as unknown) as
      | Record<string, unknown>
      | undefined;
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

function isEnabledHostBoolean(value: unknown): boolean {
  // Lynx iOS exposes native BOOL global props to the JS runtime as 1. Keep
  // the accepted contract narrow so strings and arbitrary truthy values
  // cannot enable a host capability.
  return value === true || value === 1;
}

/**
 * Returns true when the host explicitly advertises callback-driven route
 * ownership. Reading a trusted global prop avoids probing native module
 * methods, which can produce FUNCTION_NOT_FOUND diagnostics on other hosts.
 */
export function supportsExplicitRouteOwnership(): boolean {
  'background only';
  return isEnabledHostBoolean(
    getGlobalProps()?.explorerSupportsExplicitRouteOwnership
  );
}

/** Returns true when this Explorer host can create a Sparkling container. */
export function supportsSparklingContainer(): boolean {
  'background only';
  return isEnabledHostBoolean(
    getGlobalProps()?.explorerSupportsSparklingContainer
  );
}

/**
 * Returns true if the Sparkling SDK is loaded and the spkPipe module is
 * registered. Used for informational display (e.g. Settings page).
 */
export function isSparklingAvailable(): boolean {
  const props = getGlobalProps();
  const hasNativeSignal =
    isEnabledHostBoolean(props?.sparklingAvailable) ||
    isEnabledHostBoolean(props?.sparklingNavigation) ||
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
  return (
    isSparklingAvailable() && isEnabledHostBoolean(props?.sparklingNavigation)
  );
}

function isDirectCanonicalSparklingScheme(url: string): boolean {
  return /^hybrid:\/\/lynxview_page(?:[/?#]|$)/i.test(url.trim());
}

function explorerModule(): ExplorerNativeModule {
  'background only';
  return NativeModules.ExplorerModule as ExplorerNativeModule;
}

function routeError(
  action: string,
  code: string | number | undefined,
  message: string | undefined
): Error {
  const suffix = code === undefined ? '' : ` (${code})`;
  return new Error(
    `[navigation] ${action} failed${suffix}: ${message || 'Unknown error'}`
  );
}

function callbackRoute(
  action: string,
  invoke: (callback: NativeRouteCallback) => void
): Promise<void> {
  'background only';
  return new Promise((resolve, reject) => {
    try {
      invoke((result) => {
        if (result?.success === true) {
          resolve();
          return;
        }
        const error = routeError(action, result?.code, result?.message);
        console.error(error.message);
        reject(error);
      });
    } catch (cause) {
      const error =
        cause instanceof Error
          ? cause
          : routeError(action, undefined, String(cause));
      console.error(error.message);
      reject(error);
    }
  });
}

function rejectedRoute(
  action: string,
  code: string,
  message: string
): Promise<void> {
  'background only';
  const error = routeError(action, code, message);
  console.error(error.message);
  return Promise.reject(error);
}

function legacyRoute(action: string, invoke: () => void): Promise<void> {
  'background only';
  try {
    invoke();
    return Promise.resolve();
  } catch (cause) {
    const error =
      cause instanceof Error
        ? cause
        : routeError(action, undefined, String(cause));
    console.error(error.message);
    return Promise.reject(error);
  }
}

function sparklingRoute(
  action: string,
  invoke: (callback: (result: SparklingRouteResult) => void) => void
): Promise<void> {
  'background only';
  return new Promise((resolve, reject) => {
    try {
      invoke((result) => {
        if (result?.code === 1) {
          resolve();
          return;
        }
        const error = routeError(action, result?.code, result?.msg);
        console.error(error.message);
        reject(error);
      });
    } catch (cause) {
      const error =
        cause instanceof Error
          ? cause
          : routeError(action, undefined, String(cause));
      console.error(error.message);
      reject(error);
    }
  });
}

function openWithNativeCoordinator(
  url: string,
  container: ContainerRequest
): Promise<void> {
  'background only';
  if (container === 'sparkling' && !supportsSparklingContainer()) {
    return rejectedRoute(
      'native open',
      'capability_unavailable',
      'This Explorer host was built without Sparkling container support.'
    );
  }
  if (!supportsExplicitRouteOwnership()) {
    if (container === 'sparkling') {
      return rejectedRoute(
        'native open',
        'capability_unavailable',
        'ExplorerModule.openRoute is unavailable; explicit Sparkling ownership requires a host coordinator.'
      );
    }
    return legacyRoute('native open', () => {
      explorerModule().openSchema(url);
    });
  }

  const module = explorerModule();
  const openRoute = module.openRoute;
  if (typeof openRoute === 'function') {
    return callbackRoute('native open', (callback) => {
      openRoute.call(module, url, container, callback);
    });
  }
  if (container === 'sparkling') {
    return rejectedRoute(
      'native open',
      'capability_unavailable',
      'ExplorerModule.openRoute is unavailable; explicit Sparkling ownership requires a host coordinator.'
    );
  }
  return legacyRoute('native open', () => {
    module.openSchema(url);
  });
}

function openWithSparklingRouter(url: string): Promise<void> {
  'background only';
  const scheme = url.trim();
  return sparklingRoute('Sparkling open', (callback) => {
    sparklingOpen({ scheme }, callback);
  });
}

/**
 * Opens a URL with semantic container ownership. Only a direct hybrid scheme
 * may stay on an active Sparkling router. Raw resources and wrappers go through
 * the native coordinator so one parser owns classification and canonicalization.
 * History is written only after acceptance.
 */
export function openSchema(
  url: string,
  container: ContainerRequest = 'automatic',
  source: LaunchSessionSource = 'input'
): Promise<void> {
  'background only';
  const pending =
    isSparkling() &&
    container !== 'legacy' &&
    isDirectCanonicalSparklingScheme(url)
      ? openWithSparklingRouter(url)
      : openWithNativeCoordinator(url, container);
  return pending.then(() => {
    const runtime: ExplorerRuntime =
      container === 'sparkling' || runtimeRequiredByURL(url) === 'sparkling'
        ? 'sparkling'
        : 'lynx';
    addRecentSession(url, runtime, source);
  });
}

export function openWithSparkling(url: string): Promise<void> {
  'background only';
  return openSchema(url, 'sparkling');
}

/**
 * Navigate to a bundle path with optional params. Uses sparkling-navigation's
 * navigate() inside a Sparkling container and the native coordinator's Legacy
 * route elsewhere. A Sparkling rejection is reported and never retried.
 */
export function navigateTo(
  path: string,
  params?: Record<string, string | number>
): Promise<void> {
  'background only';
  let url = `file://lynx?local://${path}`;
  if (params) {
    const qs = Object.entries(params)
      .map(
        ([key, value]) =>
          `${encodeURIComponent(key)}=${encodeURIComponent(String(value))}`
      )
      .join('&');
    url += `?${qs}`;
  }

  if (isSparkling()) {
    return sparklingRoute('Sparkling navigate', (callback) => {
      sparklingNavigate(
        {
          path,
          options: params ? { params } : undefined,
        },
        callback
      );
    });
  }
  return openWithNativeCoordinator(url, 'legacy');
}

/**
 * Close the current page. In a Sparkling container uses sparkling-navigation's
 * close(). Otherwise uses NativeModules.ExplorerModule.navigateBack().
 */
export function navigateBack(): Promise<void> {
  'background only';
  if (isSparkling()) {
    return sparklingRoute('Sparkling close', (callback) => {
      sparklingClose({}, callback);
    });
  }
  if (!supportsExplicitRouteOwnership()) {
    return rejectedRoute(
      'native close',
      'capability_unavailable',
      'ExplorerModule.navigateBack is unavailable on this host.'
    );
  }
  const module = explorerModule();
  const navigateBack = module.navigateBack;
  if (typeof navigateBack !== 'function') {
    return rejectedRoute(
      'native close',
      'capability_unavailable',
      'ExplorerModule.navigateBack is unavailable on this host.'
    );
  }
  return callbackRoute('native close', (callback) => {
    navigateBack.call(module, callback);
  });
}
