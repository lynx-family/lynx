// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxContext } from '../../tasm/LynxContext';

export class HttpStreamingDelegate {
  private streamingId: string;
  private context: LynxContext;
  private sseBuffer: ArrayBuffer = new ArrayBuffer(0);
  private pendingBuffer: ArrayBuffer = new ArrayBuffer(0);

  constructor(streamingId: string, context: LynxContext) {
    this.streamingId = streamingId;
    this.context = context;
  }

  onData(data: ArrayBuffer): void {
    const res: Record<string, Object> = {'event': 'onData', 'data': data};
    this.context.sendGlobalEvent(this.streamingId, [res]);
  }

  onDataSSE(data: ArrayBuffer): void {
    const merged = new Uint8Array(this.mergeArrayBuffer(this.sseBuffer, data));

    let chunkStart = 0;
    let prev = -1;
    let sawCR = false;
    for (let index = 0; index < merged.byteLength; index++) {
      const curr = merged[index];
      if (curr == 13) { /* \r */
        if (!sawCR) {
          sawCR = true;
        } else {
          prev = -1;
        }
        continue;
      }
      sawCR = false;
      if (prev == 10 && curr == 10) {
        this.onData(merged.buffer.slice(chunkStart, index + 1));
        chunkStart = index + 1;
        prev = -1;
        continue;
      }
      prev = curr;
    }

    this.sseBuffer = merged.buffer.slice(chunkStart);
  }

  onDataPending(data: ArrayBuffer): void {
    this.pendingBuffer = this.mergeArrayBuffer(this.pendingBuffer, data);
  }

  flushPendingData(useSSE: boolean): void {
    if (this.pendingBuffer.byteLength == 0) {
      return;
    }
    if (useSSE) {
      this.onDataSSE(this.pendingBuffer);
    } else {
      this.onData(this.pendingBuffer);
    }
    this.pendingBuffer = new ArrayBuffer(0);
  }

  flushSSEBuffer(): void {
    if (this.sseBuffer.byteLength > 0) {
      this.onData(this.sseBuffer);
      this.sseBuffer = new ArrayBuffer(0);
    }
  }

  onEnd(): void {
    const res: Record<string, Object> = {'event': 'onEnd'};
    this.context.sendGlobalEvent(this.streamingId, [res]);
  }

  onError(error: string): void {
    const res: Record<string, Object> = {'event': 'onError', 'error': error};
    this.context.sendGlobalEvent(this.streamingId, [res]);
  }

  private mergeArrayBuffer(first: ArrayBuffer, second: ArrayBuffer): ArrayBuffer {
    const merged = new Uint8Array(first.byteLength + second.byteLength);
    merged.set(new Uint8Array(first));
    merged.set(new Uint8Array(second), first.byteLength);
    return merged.buffer;
  }
}
