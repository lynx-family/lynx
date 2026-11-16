// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { parseLaunchCommand, runtimeRequiredByURL } from './launchCommand';
import type { ExplorerRuntime } from './launchCommand';

const STORAGE_KEY = 'explorer_launch_sessions_v1';
const LEGACY_STORAGE_KEY = 'explorer_recent_urls';
const NATIVE_STORAGE_KEY = 'explorerNativeLaunchSessionsV1';
const MAX_ITEMS = 20;

export type LaunchSessionSource = 'input' | 'scan' | 'showcase' | 'external';

export interface LaunchSession {
  readonly id: string;
  readonly url: string;
  readonly runtime: ExplorerRuntime;
  readonly source: LaunchSessionSource;
  readonly openedAt: number;
  readonly fullscreen: boolean;
  readonly hiddenNav: boolean;
  readonly theme: 'dark' | 'light' | null;
}

let memoryCache: LaunchSession[] | null = null;

function getStorage(key: string): unknown {
  try {
    const value = (lynx as any).getStorageSync(key);
    return typeof value === 'string' ? JSON.parse(value) : value;
  } catch {
    return undefined;
  }
}

function isSession(value: unknown): value is LaunchSession {
  if (!value || typeof value !== 'object') return false;
  const session = value as Partial<LaunchSession>;
  return (
    typeof session.id === 'string' &&
    typeof session.url === 'string' &&
    (session.runtime === 'lynx' || session.runtime === 'sparkling') &&
    (session.source === 'input' ||
      session.source === 'scan' ||
      session.source === 'showcase' ||
      session.source === 'external') &&
    typeof session.openedAt === 'number' &&
    typeof session.fullscreen === 'boolean' &&
    typeof session.hiddenNav === 'boolean' &&
    (session.theme === null ||
      session.theme === 'dark' ||
      session.theme === 'light')
  );
}

function migrateLegacyHistory(): LaunchSession[] {
  const value = getStorage(LEGACY_STORAGE_KEY);
  if (!Array.isArray(value)) return [];
  const now = Date.now();
  return value
    .filter((url): url is string => typeof url === 'string')
    .map((url, index) => {
      const runtime = runtimeRequiredByURL(url) ?? 'lynx';
      const command = parseLaunchCommand(url, runtime);
      return {
        id: `migrated-${index}-${url}`,
        url,
        runtime,
        source: 'input',
        openedAt: now - index,
        fullscreen: command.fullscreen,
        hiddenNav: command.hiddenNav,
        theme: command.theme,
      };
    });
}

function readStorage(): LaunchSession[] {
  if (memoryCache !== null) return memoryCache;
  const stored = getStorage(STORAGE_KEY);
  memoryCache = Array.isArray(stored)
    ? stored.filter(isSession)
    : migrateLegacyHistory();
  if (!Array.isArray(stored) && memoryCache.length) writeStorage(memoryCache);
  return memoryCache;
}

function writeStorage(sessions: LaunchSession[]): void {
  memoryCache = sessions;
  try {
    (lynx as any).setStorageSync(STORAGE_KEY, JSON.stringify(sessions));
  } catch {
    // In-memory history still works on hosts without synchronous storage.
  }
}

export function getRecentSessions(): LaunchSession[] {
  const sessions = [...readStorage(), ...readNativeSessions()].sort(
    (left, right) => right.openedAt - left.openedAt
  );
  const seen = new Set<string>();
  return sessions
    .filter((session) => {
      const key = `${session.runtime}\n${session.url}`;
      if (seen.has(key)) return false;
      seen.add(key);
      return true;
    })
    .slice(0, MAX_ITEMS);
}

function readNativeSessions(): LaunchSession[] {
  try {
    if (typeof NativeModules === 'undefined') return [];
    const value = NativeModules.ExplorerModule.readFromLocalStorage(
      NATIVE_STORAGE_KEY
    );
    if (typeof value !== 'string') return [];
    const parsed: unknown = JSON.parse(value);
    return Array.isArray(parsed) ? parsed.filter(isSession) : [];
  } catch {
    return [];
  }
}

export function addRecentSession(
  url: string,
  runtime: ExplorerRuntime,
  source: LaunchSessionSource = 'input'
): LaunchSession[] {
  const command = parseLaunchCommand(url, runtime);
  const session: LaunchSession = {
    id: `${Date.now()}-${runtime}-${url}`,
    url,
    runtime,
    source,
    openedAt: Date.now(),
    fullscreen: command.fullscreen,
    hiddenNav: command.hiddenNav,
    theme: command.theme,
  };
  const next = [
    session,
    ...readStorage().filter(
      (item) => item.url !== url || item.runtime !== runtime
    ),
  ].slice(0, MAX_ITEMS);
  writeStorage(next);
  return [...next];
}

export function clearRecentSessions(): void {
  writeStorage([]);
  try {
    NativeModules.ExplorerModule.saveToLocalStorage(NATIVE_STORAGE_KEY, '[]');
  } catch {
    // Non-iOS hosts may not expose native preference storage.
  }
}

// Compatibility for callers that only need the URL projection. New product
// surfaces should retain the full LaunchSession instead.
export function getRecentUrls(): string[] {
  return getRecentSessions().map(({ url }) => url);
}

export function addRecentUrl(url: string): string[] {
  const runtime = runtimeRequiredByURL(url) ?? 'lynx';
  return addRecentSession(url, runtime).map((session) => session.url);
}

export function clearRecentUrls(): void {
  clearRecentSessions();
}
