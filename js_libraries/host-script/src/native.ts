// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import type { HostScriptError, HostScriptErrorCode } from './types';

interface NativeHostScript {
  reportEntryResult(status: string, message?: string): void;
  hasCurrent(): boolean;
  waitForCurrent(): Promise<void>;
  loadURL(url: string, data?: string, globalProps?: string): boolean;
  loadTemplate(
    template: ArrayBuffer,
    url: string,
    initialData?: string,
    globalProps?: string,
    processor?: string,
    readOnly?: boolean
  ): boolean;
  loadSSR(data: ArrayBuffer, url: string, initialData?: string): boolean;
  hydrateSSR(data: ArrayBuffer, url: string, initialData?: string): boolean;
  updateMetaData(data?: string, globalProps?: string): boolean;
  setGlobalProps(globalProps: string): boolean;
  reloadTemplate(data?: string, globalProps?: string): boolean;
  sendGlobalEvent(name: string, params: string): boolean;
  on(event: string, listener: (...args: never[]) => void): void;
  off(event: string, listener: (...args: never[]) => void): void;
}

declare const lynx: { loadModule(name: string, target: object): void };
let module: NativeHostScript | undefined;
const methods = [
  'reportEntryResult',
  'hasCurrent',
  'waitForCurrent',
  'loadURL',
  'loadTemplate',
  'loadSSR',
  'hydrateSSR',
  'updateMetaData',
  'setGlobalProps',
  'reloadTemplate',
  'sendGlobalEvent',
  'on',
  'off',
] as const;

export function hostScriptError(
  code: HostScriptErrorCode,
  message: string
): HostScriptError {
  return Object.defineProperty(new Error(message), 'code', {
    value: code,
    enumerable: true,
  }) as HostScriptError;
}

export function native(): NativeHostScript {
  if (!module) {
    const target: Partial<NativeHostScript> = {};
    try {
      lynx.loadModule('host_script', target);
      if (methods.some((name) => typeof target[name] !== 'function'))
        throw new Error('incomplete native API');
    } catch {
      throw hostScriptError(
        'UNSUPPORTED',
        'Host Script native API is unavailable'
      );
    }
    module = target as NativeHostScript;
  }
  return module;
}

export function checkOptions<T extends object>(value: T): T {
  if (!value || typeof value !== 'object' || Array.isArray(value)) {
    throw hostScriptError('INVALID_ARGUMENT', 'Options must be an object');
  }
  return value;
}

/** Keep the native JSON ABI private; serialization failures throw synchronously. */
export function json(
  value: object | undefined,
  array = false
): string | undefined {
  if (value === undefined && !array) return undefined;
  try {
    if (!value || typeof value !== 'object' || Array.isArray(value) !== array) {
      throw new Error('invalid data');
    }
    const result = JSON.stringify(value);
    if (result === undefined) throw new Error('invalid data');
    return result;
  } catch {
    throw hostScriptError(
      'INVALID_ARGUMENT',
      array
        ? 'Event params must be a JSON-serializable array'
        : 'Page data must be a JSON-serializable object'
    );
  }
}

export function invoke(action: () => Promise<void>): Promise<void> {
  return Promise.resolve().then(action);
}
