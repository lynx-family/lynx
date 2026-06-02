// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

interface LynxElementRenderer {
  isLynxElementRendererDestroyed(): boolean;
  nativeLynxElementToJSONString(
    sign: number,
    callback: (result: string | null) => void
  ): void;
}

interface LynxElementData {
  templateRendererRef: WeakRef<LynxElementRenderer> | null;
  sign: number;
}

const lynxElementData = new WeakMap<LynxElement, LynxElementData>();

export function createLynxElement(
  templateRenderer: LynxElementRenderer | null,
  sign: number
): LynxElement {
  const element = new LynxElement();
  lynxElementData.set(element, {
    templateRendererRef: templateRenderer ? new WeakRef(templateRenderer) : null,
    sign
  });
  return element;
}

export class LynxElement {
  /**
   * Asynchronously serializes this LynxElement tree to a JSON string.
   * The callback is always invoked asynchronously on the UI thread.
   */
  toJSONString(callback: (result: string | null) => void): void {
    if (typeof callback !== 'function') {
      return;
    }
    const data = lynxElementData.get(this);
    const templateRenderer = data?.templateRendererRef?.deref();
    if (
      !data ||
      !templateRenderer ||
      templateRenderer.isLynxElementRendererDestroyed() ||
      data.sign === 0
    ) {
      setTimeout(() => {
        callback(null);
      }, 0);
      return;
    }
    templateRenderer.nativeLynxElementToJSONString(data.sign, callback);
  }
}
