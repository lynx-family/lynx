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

export class TextDecoder {
  constructor() {}

  decode(buffer: ArrayBuffer | TypedArray | DataView): string {
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
