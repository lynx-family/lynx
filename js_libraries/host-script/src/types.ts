// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import type { LynxUpdateMeta } from './data';

export type HostScriptErrorCode =
  | 'INVALID_ARGUMENT'
  | 'INVALID_STATE'
  | 'INVALID_ENTRY'
  | 'UNSUPPORTED'
  | 'PLATFORM_ERROR';

export interface HostScriptError extends Error {
  readonly code: HostScriptErrorCode;
}

/** JSON-serializable page data. */
export type LynxData = Record<string, unknown>;

export interface LynxLoadOptions {
  readonly data?: LynxData;
  readonly globalProps?: LynxData;
}

export interface LynxViewTemplateOptions {
  readonly template: ArrayBuffer;
  readonly url: string;
  readonly initialData?: LynxData;
  readonly globalProps?: LynxData;
  readonly processor?: string;
  readonly readOnly?: boolean;
}

export interface LynxSsrOptions {
  readonly data: ArrayBuffer;
  readonly url: string;
  readonly initialData?: LynxData;
}

export interface LynxReloadTemplateOptions {
  readonly data?: LynxData;
  readonly globalProps?: LynxData;
}

export interface LynxViewEventMap {
  ready: () => void;
  loadSuccess: () => void;
  firstScreen: () => void;
  pageUpdate: () => void;
  dataUpdated: () => void;
  error: (code: number, message: string) => void;
  destroyed: () => void;
}

/** A non-owning reference to this runtime's single platform-created View. */
export interface LynxView {
  readonly available: boolean;
  /** Lazily waits for the first binding; it does not wait for page loading. */
  readonly ready: Promise<void>;
  /** Returns whether the request was accepted. Observe events for page results. */
  loadURL(url: string, options?: LynxLoadOptions): boolean;
  loadTemplate(options: LynxViewTemplateOptions): boolean;
  loadSSR(options: LynxSsrOptions): boolean;
  hydrateSSR(options: LynxSsrOptions): boolean;
  updateMetaData(meta: LynxUpdateMeta): boolean;
  setGlobalProps(globalProps: LynxData): boolean;
  reloadTemplate(options?: LynxReloadTemplateOptions): boolean;
  sendGlobalEvent(name: string, ...args: unknown[]): boolean;
  /** Requires an available View. Returns this reference for chaining. */
  on<K extends keyof LynxViewEventMap>(
    event: K,
    listener: LynxViewEventMap[K]
  ): this;
  /** Removes the last matching registration; safe after View destruction. */
  off<K extends keyof LynxViewEventMap>(
    event: K,
    listener: LynxViewEventMap[K]
  ): this;
}

export interface HostScriptContext {
  readonly lynxView: LynxView;
}

export type HostScriptSetup = (
  context: HostScriptContext
) => void | PromiseLike<void>;
