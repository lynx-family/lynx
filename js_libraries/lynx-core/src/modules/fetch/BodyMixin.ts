// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// host object
export interface BodyNative {
  BodyNative(bodyInit?: BodyInit): BodyNative;
  readonly arrayBuffer: ArrayBuffer;
  readonly text: string;
  readonly json: any;
  readonly bodyUsed: boolean;
  readonly clone: BodyNative;

  // TODO(huzhanbo.luc): these APIs rely on foundamental types
  // which require extra works to support, we will support these
  // later when we have implemented these types.

  // blob(): Blob;
  // formData(): FormData;
  // cloneStream(): ReadableStream;
}

type CreateBodyNative = (body?: any) => BodyNative;

export class BodyMixin {
  _bodyData: BodyNative = null;

  constructor() {}

  protected setBody(body?: BodyInit | BodyMixin) {
    if (body instanceof BodyMixin) {
      this._bodyData = body._bodyData.clone;
    } else {
      let bodyInitNative = {
        bodyData: body,
        isArrayBuffer: false,
      };

      if (body instanceof ArrayBuffer) {
        bodyInitNative.isArrayBuffer = true;
      } else if (body instanceof DataView) {
        bodyInitNative.isArrayBuffer = true;
        bodyInitNative.bodyData = body.buffer.slice(
          body.byteOffset,
          body.byteOffset + body.byteLength
        );
      } else if (ArrayBuffer.isView(body)) {
        bodyInitNative.isArrayBuffer = true;
        bodyInitNative.bodyData = body.buffer;
      } else if (
        globalThis.URLSearchParams &&
        body instanceof URLSearchParams
      ) {
        bodyInitNative.bodyData = body.toString();
      }

      this._bodyData = globalThis.CreateBodyNative(bodyInitNative);
    }
  }

  public arrayBuffer(): Promise<ArrayBuffer> {
    return Promise.resolve(this._bodyData.arrayBuffer);
  }

  public text(): Promise<string> {
    return Promise.resolve(this._bodyData.text);
  }

  public json(): Promise<any> {
    return Promise.resolve(this._bodyData.json);
  }

  get bodyUsed() {
    return this._bodyData.bodyUsed;
  }
}
