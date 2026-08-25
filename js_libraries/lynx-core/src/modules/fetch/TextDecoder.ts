type TypedArray =
  | Int8Array
  | Uint8Array
  | Uint8ClampedArray
  | Int16Array
  | Uint16Array
  | Int32Array
  | Uint32Array
  | Float32Array
  | Float64Array;

type DecodeOptions = {
  stream?: boolean;
};

const REPLACEMENT_CHARACTER = '\uFFFD';

function isContinuationByte(byte: number): boolean {
  return byte >= 0x80 && byte <= 0xbf;
}

function utf8SequenceLength(leadingByte: number): number {
  if (leadingByte >= 0xc2 && leadingByte <= 0xdf) {
    return 2;
  }
  if (leadingByte >= 0xe0 && leadingByte <= 0xef) {
    return 3;
  }
  if (leadingByte >= 0xf0 && leadingByte <= 0xf4) {
    return 4;
  }
  return 1;
}

function isValidUtf8Prefix(
  bytes: Uint8Array,
  start: number,
  end: number,
  expectedLength: number
): boolean {
  if (expectedLength === 1) {
    return false;
  }

  const leadingByte = bytes[start];
  for (let index = start + 1; index < end; index++) {
    const byte = bytes[index];
    if (!isContinuationByte(byte)) {
      return false;
    }

    if (index === start + 1) {
      if (leadingByte === 0xe0 && byte < 0xa0) {
        return false;
      }
      if (leadingByte === 0xed && byte > 0x9f) {
        return false;
      }
      if (leadingByte === 0xf0 && byte < 0x90) {
        return false;
      }
      if (leadingByte === 0xf4 && byte > 0x8f) {
        return false;
      }
    }
  }

  return end - start < expectedLength;
}

function incompleteUtf8SuffixLength(bytes: Uint8Array): number {
  const length = bytes.length;
  if (length === 0) {
    return 0;
  }

  let start = length - 1;
  while (start >= 0 && isContinuationByte(bytes[start])) {
    start--;
  }

  if (start < 0) {
    return 0;
  }

  const expectedLength = utf8SequenceLength(bytes[start]);
  if (
    expectedLength > 1 &&
    length - start < expectedLength &&
    isValidUtf8Prefix(bytes, start, length, expectedLength)
  ) {
    return length - start;
  }

  return 0;
}

function toBytes(buffer?: ArrayBuffer | TypedArray | DataView): Uint8Array {
  if (buffer === undefined) {
    return new Uint8Array(0);
  }

  if (buffer instanceof DataView) {
    return new Uint8Array(buffer.buffer, buffer.byteOffset, buffer.byteLength);
  }

  if (ArrayBuffer.isView(buffer)) {
    return new Uint8Array(buffer.buffer, buffer.byteOffset, buffer.byteLength);
  }

  return new Uint8Array(buffer);
}

function concatBytes(left: Uint8Array, right: Uint8Array): Uint8Array {
  if (left.length === 0) {
    return right;
  }
  if (right.length === 0) {
    return left;
  }

  const bytes = new Uint8Array(left.length + right.length);
  bytes.set(left, 0);
  bytes.set(right, left.length);
  return bytes;
}

function sliceBytes(
  bytes: Uint8Array,
  start: number,
  end: number = bytes.length
): Uint8Array {
  const view = bytes.subarray(start, end);
  const copy = new Uint8Array(view.length);
  copy.set(view);
  return copy;
}

function bytesToArrayBuffer(bytes: Uint8Array): ArrayBuffer {
  if (bytes.byteOffset === 0 && bytes.byteLength === bytes.buffer.byteLength) {
    return bytes.buffer;
  }

  return sliceBytes(bytes, 0).buffer;
}

function decodeBytes(bytes: Uint8Array): string {
  if (bytes.length === 0) {
    return '';
  }

  return globalThis.TextCodecHelper.decode(bytesToArrayBuffer(bytes));
}

function decodeLegacyBuffer(
  buffer: ArrayBuffer | TypedArray | DataView
): string {
  if (buffer.byteLength === 0) {
    return '';
  }

  if (buffer instanceof DataView) {
    buffer = buffer.buffer.slice(
      buffer.byteOffset,
      buffer.byteOffset + buffer.byteLength
    );
  } else if (ArrayBuffer.isView(buffer)) {
    buffer = buffer.buffer;
  }

  return globalThis.TextCodecHelper.decode(buffer);
}

export class TextDecoder {
  private pendingBytes: Uint8Array = new Uint8Array(0);

  constructor() {}

  decode(
    buffer?: ArrayBuffer | TypedArray | DataView,
    options?: DecodeOptions
  ): string {
    const stream = options?.stream === true;
    if (!stream && this.pendingBytes.length === 0 && buffer !== undefined) {
      return decodeLegacyBuffer(buffer);
    }

    const bytes = concatBytes(this.pendingBytes, toBytes(buffer));
    const pendingLength = incompleteUtf8SuffixLength(bytes);

    if (stream) {
      this.pendingBytes =
        pendingLength === 0
          ? new Uint8Array(0)
          : sliceBytes(bytes, bytes.length - pendingLength);
      return decodeBytes(
        pendingLength === 0
          ? bytes
          : sliceBytes(bytes, 0, bytes.length - pendingLength)
      );
    }

    this.pendingBytes = new Uint8Array(0);
    if (pendingLength === 0) {
      return decodeBytes(bytes);
    }

    return (
      decodeBytes(sliceBytes(bytes, 0, bytes.length - pendingLength)) +
      REPLACEMENT_CHARACTER
    );
  }

  encodeInto() {
    throw TypeError('TextEncoder().encodeInto not supported');
  }

  get encoding() {
    return 'utf-8';
  }

  get fatal() {
    return false;
  }

  get ignoreBOM() {
    return true;
  }
}
