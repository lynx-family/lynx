// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { TextDecoder } from './TextDecoder';
import { TextEncoder } from './TextEncoder';
export class BodyMixin {
  _arrayBuffer: ArrayBuffer;
  _bodyUsed: boolean;

  constructor() {
    this._arrayBuffer = new ArrayBuffer(0);
    this._bodyUsed = false;
  }

  private safeUseBody<T>(use: (body: ArrayBuffer) => T): T {
    if (this._bodyUsed) {
      // TODO(huzhanbo.luc): throw a error if the break change is ok.
      return undefined;
    }

    const ret = use(this._arrayBuffer);
    this._bodyUsed = true;
    this._arrayBuffer = null;
    return ret;
  }

  private cloneArrayBuffer(src: ArrayBuffer) {
    return src.slice(0);
  }

  protected setBody(body?: BodyInit | BodyMixin) {
    if (body instanceof BodyMixin) {
      if (body._bodyUsed) {
        // TODO(huzhanbo.luc): throw a error if the break change is ok.
        return;
      }
      this._arrayBuffer = this.cloneArrayBuffer(body._arrayBuffer);
    } else {
      if (body instanceof ArrayBuffer) {
        this._arrayBuffer = this.cloneArrayBuffer(body);
      } else if (body instanceof DataView) {
        this._arrayBuffer = this.cloneArrayBuffer(
          body.buffer.slice(body.byteOffset, body.byteOffset + body.byteLength)
        );
      } else if (ArrayBuffer.isView(body)) {
        this._arrayBuffer = this.cloneArrayBuffer(body.buffer);
      } else if (body) {
        this._arrayBuffer = new TextEncoder().encode(body.toString()).buffer;
      }
    }
  }

  public arrayBuffer(): Promise<ArrayBuffer> {
    return Promise.resolve(this.safeUseBody((body) => body));
  }

  public text(): Promise<string> {
    return Promise.resolve(
      this.safeUseBody((body) => new TextDecoder().decode(body))
    );
  }

  public json(): Promise<any> {
    return Promise.resolve(
      this.safeUseBody((body) => JSON.parse(new TextDecoder().decode(body)))
    );
  }

  // TODO(huzhanbo.luc): these APIs rely on foundamental types
  // which require extra works to support, we will support these
  // later when we have implemented these types.

  // blob(): Blob;
  // formData(): FormData;
  // cloneStream(): ReadableStream;

  public get bodyUsed() {
    return this._bodyUsed;
  }
}
