// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import type { LynxTemplateRenderer } from './LynxTemplateRenderer';

export class LynxElement {
  private templateRendererRef: WeakRef<LynxTemplateRenderer> | null = null;
  private sign: number = 0;

  protected constructor(templateRenderer: LynxTemplateRenderer | null, sign: number) {
    if (templateRenderer) {
      this.templateRendererRef = new WeakRef(templateRenderer);
    }
    this.sign = sign;
  }

  /**
   * Serializes this LynxElement tree to a JSON string.
   */
  toJSONString(callback: (result: string | null) => void): void {
    if (typeof callback !== 'function') {
      return;
    }
    const templateRenderer = this.templateRendererRef?.deref();
    if (
      !templateRenderer ||
      templateRenderer.isDestroyed() ||
      this.sign === 0
    ) {
      callback(null);
      return;
    }
    templateRenderer.nativeLynxElementToJSONString(this.sign, (result: string | null) => {
      callback(result && result.length > 0 ? result : null);
    });
  }
}

/** @internal */
export class LynxElementImpl extends LynxElement {
  constructor(templateRenderer: LynxTemplateRenderer | null, sign: number) {
    super(templateRenderer, sign);
  }
}
