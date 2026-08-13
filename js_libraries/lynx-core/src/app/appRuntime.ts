// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import nativeGlobal from '../common/nativeGlobal';

/**
 * App-runtime level globals for a shared JS context.
 *
 * Timers, microtasks, and rAF used to be handed to modules as the loading
 * card's native bindings. A module in a shared chunk then captured the
 * first card's binding, and once that card was destroyed every later call
 * silently no-oped (the native side guards with `IsDestroying`), which is
 * also why a captured `lynx.Promise` stopped settling.
 *
 * This registry gives the whole context one stable set of timer functions.
 * Calls route to a live driver at call time — preferably the standalone
 * App runtime (a background runtime that is not attached to any LynxView
 * and outlives every card), otherwise any live card. Pending timers armed
 * through a dying driver are re-armed on a surviving one with their
 * remaining delay.
 */

export interface AppRuntimeDriver {
  /** Stable id for bookkeeping. */
  id: string;
  /** Standalone App runtime: preferred and assumed long-lived. */
  durable: boolean;
  setTimeout(cb: () => void, delay?: number): unknown;
  clearTimeout(id: unknown): void;
  setInterval(cb: () => void, delay: number): unknown;
  clearInterval(id: unknown): void;
  queueMicrotask?(cb: () => void): void;
  requestAnimationFrame?(cb: () => void): unknown;
  cancelAnimationFrame?(id: unknown): void;
  onUnhandledRejection?(reason: Error): void;
}

interface PendingTimer {
  token: number;
  driverId: string;
  nativeId: unknown;
  cb: () => void;
  /** Absolute deadline for timeouts; undefined for intervals. */
  fireAt?: number;
  /** Interval period; undefined for timeouts. */
  interval?: number;
}

interface AppRuntimeState {
  drivers: AppRuntimeDriver[];
  pending: Map<number, PendingTimer>;
  nextToken: number;
  globals?: AppRuntimeGlobals;
  promiseCtor?: PromiseConstructor;
}

export interface AppRuntimeGlobals {
  setTimeout(cb: () => void, delay?: number): number;
  clearTimeout(token?: number): void;
  setInterval(cb: () => void, delay: number): number;
  clearInterval(token?: number): void;
  queueMicrotask(cb: () => void): void;
  requestAnimationFrame(cb: () => void): unknown;
  cancelAnimationFrame(id: unknown): void;
}

function state(): AppRuntimeState {
  return (nativeGlobal._$appRuntime ??= {
    drivers: [],
    pending: new Map(),
    nextToken: 1,
  } as AppRuntimeState);
}

function bestDriver(): AppRuntimeDriver | undefined {
  const { drivers } = state();
  // Prefer the standalone App runtime; it survives every card.
  return drivers.find((d) => d.durable) ?? drivers[0];
}

function armTimeout(s: AppRuntimeState, entry: PendingTimer): void {
  const driver = bestDriver();
  if (!driver) {
    // No live driver right now (e.g. every card closed and no standalone
    // runtime). Keep the entry; it is re-armed when a driver registers.
    entry.driverId = '';
    entry.nativeId = undefined;
    return;
  }
  const delay = Math.max(0, (entry.fireAt ?? Date.now()) - Date.now());
  entry.driverId = driver.id;
  entry.nativeId = driver.setTimeout(() => {
    s.pending.delete(entry.token);
    entry.cb();
  }, delay);
}

function armInterval(s: AppRuntimeState, entry: PendingTimer): void {
  const driver = bestDriver();
  if (!driver) {
    entry.driverId = '';
    entry.nativeId = undefined;
    return;
  }
  entry.driverId = driver.id;
  entry.nativeId = driver.setInterval(entry.cb, entry.interval!);
}

export function registerAppRuntimeDriver(driver: AppRuntimeDriver): void {
  const s = state();
  if (s.drivers.some((d) => d.id === driver.id)) {
    return;
  }
  s.drivers.push(driver);
  // Timers orphaned while no driver was alive come back to life here.
  s.pending.forEach((entry) => {
    if (entry.driverId === '') {
      entry.interval === undefined ? armTimeout(s, entry) : armInterval(s, entry);
    }
  });
}

export function unregisterAppRuntimeDriver(id: string): void {
  const s = state();
  const index = s.drivers.findIndex((d) => d.id === id);
  if (index < 0) {
    return;
  }
  const dying = s.drivers[index];
  s.drivers.splice(index, 1);
  // Move this driver's pending timers onto a survivor, keeping the
  // remaining delay for timeouts and the period for intervals.
  s.pending.forEach((entry) => {
    if (entry.driverId !== id) {
      return;
    }
    try {
      entry.interval === undefined
        ? dying.clearTimeout(entry.nativeId)
        : dying.clearInterval(entry.nativeId);
    } catch (e) {
      // The native side may already be gone; the timer is dead either way.
    }
    entry.interval === undefined ? armTimeout(s, entry) : armInterval(s, entry);
  });
}

export function getAppRuntimeGlobals(): AppRuntimeGlobals {
  const s = state();
  if (s.globals) {
    return s.globals;
  }
  const globals: AppRuntimeGlobals = {
    setTimeout(cb: () => void, delay?: number): number {
      const token = s.nextToken++;
      const entry: PendingTimer = {
        token,
        driverId: '',
        nativeId: undefined,
        cb,
        fireAt: Date.now() + Math.max(0, delay ?? 0),
      };
      s.pending.set(token, entry);
      armTimeout(s, entry);
      return token;
    },
    clearTimeout(token?: number): void {
      if (token === undefined) return;
      const entry = s.pending.get(token);
      if (!entry) return;
      s.pending.delete(token);
      const driver = s.drivers.find((d) => d.id === entry.driverId);
      driver?.clearTimeout(entry.nativeId);
    },
    setInterval(cb: () => void, delay: number): number {
      const token = s.nextToken++;
      const entry: PendingTimer = {
        token,
        driverId: '',
        nativeId: undefined,
        cb,
        interval: delay,
      };
      s.pending.set(token, entry);
      armInterval(s, entry);
      return token;
    },
    clearInterval(token?: number): void {
      if (token === undefined) return;
      const entry = s.pending.get(token);
      if (!entry) return;
      s.pending.delete(token);
      const driver = s.drivers.find((d) => d.id === entry.driverId);
      driver?.clearInterval(entry.nativeId);
    },
    queueMicrotask(cb: () => void): void {
      const driver = bestDriver();
      if (driver?.queueMicrotask) {
        driver.queueMicrotask(cb);
      } else if (driver) {
        driver.setTimeout(cb, 0);
      } else {
        // Deliver on revival rather than dropping silently.
        globals.setTimeout(cb, 0);
      }
    },
    requestAnimationFrame(cb: () => void): unknown {
      const driver = bestDriver();
      if (driver?.requestAnimationFrame) {
        return driver.requestAnimationFrame(cb);
      }
      return globals.setTimeout(cb, 16);
    },
    cancelAnimationFrame(id: unknown): void {
      const driver = bestDriver();
      if (driver?.cancelAnimationFrame) {
        driver.cancelAnimationFrame(id);
      } else {
        globals.clearTimeout(id as number);
      }
    },
  };
  s.globals = globals;
  return globals;
}

/**
 * The context-shared Promise constructor. Created once per JS context on
 * top of the App-runtime timers, so a Promise captured by a shared module
 * keeps settling after the card that created it is destroyed.
 */
export function getSharedPromiseCtor(
  create: (
    setTimeout: (cb: () => void, delay?: number) => unknown,
    clearTimeout: (id: unknown) => void,
    queueMicrotask: (cb: () => void) => void,
    onUnhandledRejection: (reason: Error) => void
  ) => PromiseConstructor
): PromiseConstructor {
  const s = state();
  if (s.promiseCtor) {
    return s.promiseCtor;
  }
  const globals = getAppRuntimeGlobals();
  s.promiseCtor = create(
    globals.setTimeout,
    globals.clearTimeout as (id: unknown) => void,
    globals.queueMicrotask,
    (reason) => {
      bestDriver()?.onUnhandledRejection?.(reason);
    }
  );
  return s.promiseCtor;
}
