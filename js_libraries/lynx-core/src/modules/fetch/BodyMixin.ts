// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { TextDecoder } from './TextDecoder';
import { TextEncoder } from './TextEncoder';
import { LynxReadableStream } from './ReadableStream';
export class BodyMixin {
  _arrayBuffer: ArrayBuffer;
  _bodyStream: LynxReadableStream;
  _bodyUsed: boolean;
  _enableFetchAPIStandardStreaming: boolean;

  constructor() {
    this._arrayBuffer = new ArrayBuffer(0);
    this._bodyStream = null;
    this._bodyUsed = false;
    this._enableFetchAPIStandardStreaming = false;
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

  protected setBody(
    body?: BodyInit | BodyMixin | ReadableStream,
    enableFetchAPIStandardStreaming?: boolean
  ) {
    if (body instanceof BodyMixin) {
      if (body._bodyUsed || body._bodyStream) {
        throw new Error('body used, or try to copy body stream');
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
      if (body instanceof LynxReadableStream) {
        this._bodyStream = body;
        this._enableFetchAPIStandardStreaming = enableFetchAPIStandardStreaming;
      }
    }
  }

  public async arrayBuffer(): Promise<ArrayBuffer> {
    if (this._enableFetchAPIStandardStreaming && this._bodyStream != null) {
      const buffer = await this.consumeStream();
      if (buffer === null) {
        return new ArrayBuffer(0);
      }
      return buffer;
    } else {
      return Promise.resolve(this.safeUseBody((body) => body));
    }
  }

  public get body() {
    if (this._bodyUsed) {
      throw new Error('body used');
    }
    this._bodyUsed = true;
    return this._bodyStream;
  }

  public async text(): Promise<string> {
    if (this._enableFetchAPIStandardStreaming && this._bodyStream != null) {
      const buffer = await this.consumeStream();
      if (buffer === null) {
        return '';
      }
      return new TextDecoder().decode(buffer);
    } else {
      const result = await this.safeUseBody((body) =>
        new TextDecoder().decode(body)
      );
      return result;
    }
  }

  public async json(): Promise<any> {
    if (this._enableFetchAPIStandardStreaming && this._bodyStream != null) {
      const buffer = await this.consumeStream();
      if (buffer === null) {
        return null;
      }
      const text = new TextDecoder().decode(buffer);
      return JSON.parse(text);
    } else {
      const result = this.safeUseBody((body) => new TextDecoder().decode(body));
      return Promise.resolve(result).then((text) => JSON.parse(text));
    }
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

  private async getArrayBufferOfStreaming(): Promise<ArrayBuffer> {
    const chunks: Uint8Array[] = [];
    let totalLength = 0;
    const reader = ((this
      ._bodyStream as unknown) as ReadableStream).getReader();
    {
      // 1. read all data chunks
      while (true) {
        const { done, value } = await reader.read();
        if (done) {
          break;
        }
        chunks.push(new Uint8Array(value));
        totalLength += value.byteLength;
      }
      // 2. create finalBuffer and merge datas
      const finalBuffer = new Uint8Array(totalLength);
      let offset = 0;
      for (const chunk of chunks) {
        finalBuffer.set(chunk, offset);
        offset += chunk.byteLength;
      }
      return finalBuffer.buffer;
    }
  }

  private async consumeStream(): Promise<ArrayBuffer | null> {
    if (this._bodyUsed) {
      return null;
    }

    this._bodyUsed = true;
    return await this.getArrayBufferOfStreaming();
  }
}
