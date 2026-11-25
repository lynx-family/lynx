// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { hostScriptError } from './native';

/** Page data is serialized when an operation is invoked. */
export class LynxTemplateData<T extends object = Record<string, unknown>> {
  constructor(private readonly value: T) {
    if (value === null || typeof value !== 'object' || Array.isArray(value)) {
      throw hostScriptError(
        'INVALID_ARGUMENT',
        'Template data must be an object'
      );
    }
  }

  toObject(): T {
    return this.value;
  }
}

export class LynxUpdateMeta {
  updateData?: LynxTemplateData<object>;
  globalProps?: LynxTemplateData<object>;

  constructor(init?: {
    updateData?: LynxTemplateData<object>;
    globalProps?: LynxTemplateData<object>;
  }) {
    this.updateData = init?.updateData;
    this.globalProps = init?.globalProps;
  }
}
