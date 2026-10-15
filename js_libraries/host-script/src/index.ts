// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { checkOptions, hostScriptError, invoke, json, native } from './native';
import type { HostScriptContext, HostScriptSetup, LynxView } from './types';
export type * from './types';
import { LynxTemplateData } from './data';
export { LynxTemplateData, LynxUpdateMeta } from './data';

let defined = false;
let ready: Promise<void> | undefined;
const lynxView: LynxView = {
  get available() {
    return native().hasCurrent();
  },
  get ready() {
    return (ready ??= invoke(() => native().waitForCurrent()));
  },
  loadURL: (url, options = {}) =>
    native().loadURL(
      url,
      json(checkOptions(options).data),
      json(options.globalProps)
    ),
  loadTemplate: (options) =>
    native().loadTemplate(
      checkOptions(options).template,
      options.url,
      json(options.initialData),
      json(options.globalProps),
      options.processor,
      options.readOnly
    ),
  loadSSR: (options) =>
    native().loadSSR(
      checkOptions(options).data,
      options.url,
      json(options.initialData)
    ),
  hydrateSSR: (options) =>
    native().hydrateSSR(
      checkOptions(options).data,
      options.url,
      json(options.initialData)
    ),
  updateMetaData: (meta) => {
    if (
      !meta ||
      typeof meta !== 'object' ||
      (meta.updateData === undefined && meta.globalProps === undefined)
    ) {
      throw hostScriptError(
        'INVALID_ARGUMENT',
        'Provide updateData or globalProps'
      );
    }
    const encode = (data: LynxTemplateData<object> | undefined) => {
      if (data === undefined) return undefined;
      if (!(data instanceof LynxTemplateData)) {
        throw hostScriptError('INVALID_ARGUMENT', 'Expected LynxTemplateData');
      }
      return json(data.toObject());
    };
    return native().updateMetaData(
      encode(meta.updateData),
      encode(meta.globalProps)
    );
  },
  setGlobalProps: (props) => {
    const encoded = json(props);
    if (encoded === undefined) {
      throw hostScriptError(
        'INVALID_ARGUMENT',
        'Global props must be an object'
      );
    }
    return native().setGlobalProps(encoded);
  },
  reloadTemplate: (options = {}) =>
    native().reloadTemplate(
      json(checkOptions(options).data),
      json(options.globalProps)
    ),
  sendGlobalEvent: (name, ...args) =>
    native().sendGlobalEvent(name, json(args, true)!),
  on(event, listener) {
    if (typeof listener !== 'function') {
      throw hostScriptError(
        'INVALID_ARGUMENT',
        'on expects an event name and listener'
      );
    }
    native().on(event, listener as (...args: never[]) => void);
    return this;
  },
  off(event, listener) {
    if (typeof listener !== 'function') {
      throw hostScriptError(
        'INVALID_ARGUMENT',
        'off expects an event name and listener'
      );
    }
    const bridge = native();
    if (bridge.hasCurrent())
      bridge.off(event, listener as (...args: never[]) => void);
    return this;
  },
};
const context: HostScriptContext = Object.freeze({
  lynxView: Object.freeze(lynxView),
});

/** Register once, synchronously in the main script. View readiness is opt-in. */
export function defineHostScript(setup: HostScriptSetup): void {
  const bridge = native();
  if (defined || typeof setup !== 'function') {
    const message = defined
      ? 'defineHostScript can only be called once'
      : 'defineHostScript expects a setup function';
    bridge.reportEntryResult('INVALID_ENTRY', message);
    throw hostScriptError('INVALID_ENTRY', message);
  }
  defined = true;
  bridge.reportEntryResult('REGISTERED');
  const failed = (reason: unknown) =>
    bridge.reportEntryResult(
      'ERROR',
      reason instanceof Error ? reason.message || reason.name : String(reason)
    );
  try {
    void Promise.resolve(setup(context)).then(
      () => bridge.reportEntryResult('READY'),
      failed
    );
  } catch (reason) {
    failed(reason);
  }
}
