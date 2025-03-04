// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

//@ts-nocheck
/**
 * This file is auto-generated, DO NOT MODIFY!!
 */
 
'use strict';

let TestContextCB;

function str2view(str, view, start) {
  for (let i = 0, strLen = str.length; i < strLen; ++i) {
    // Always little-endian to adapt to native conversion util.
    view.setInt16(start * 4 + i * 2, str.charCodeAt(i), true);
  }
}

function getViewType(view) {
  const viewType = view.constructor.name;
  switch (view.BYTES_PER_ELEMENT) {
    case 1:
      switch (viewType) {
        case 'Int8Array':
          return 1;
        case 'Uint8Array':
          return 2;
        default:
          // Uint8ClampedArray
          return 3;
      }
      break;
    case 2:
      switch (viewType) {
        case 'Int16Array':
          return 4;
        default:
          // Uint16Array
          return 5;
      }
      break;
    case 4:
      switch (viewType) {
        case 'Int32Array':
          return 6;
        case 'Uint32Array':
          return 7;
        default:
          // Float32Array
          return 8;
      }
      break;
    case 8:
      switch (viewType) {
        case 'Float64Array':
          return 9;
        case 'BigInt64Array':
          return 10;
        default:
          // BigUint64Array
          return 11;
      }
      break;
    default:
      // DataView
      return 12;
  }
}

const commandBufferCreator = function(appendTarget) {
  if (TestContextCB) return TestContextCB;
  const cb = appendTarget.getCommandBuffer();
  const buffer = cb.buffer;

  const uint8View = new Uint8Array(buffer);
  const float32View = new Float32Array(buffer);
  const int32View = new Int32Array(buffer);
  const uint32View = new Uint32Array(buffer);
  const dataView = new DataView(buffer);

  const objects = cb.objects;
  const active = cb.active;

  // Flush at 4 * 40 = 160KB, which is 80% of total buffer size.
  const flushThres = 40 * 1024;
  // Current cutoff for long commands is 32KB, which is well below the 40KB free space.
  const cmdLengthCutoff = 8192;
  const isLittleEndian = new Uint16Array(new Uint8Array([1, 0]).buffer)[0] === 1;

  // Leave 0 as invalid.
  let globalAsyncId = 1;
  let TestAsyncObject = appendTarget.TestAsyncObject

  function voidFromVoid() {
    let end = uint32View[0];
    uint32View[end] = 1;
    uint32View[end + 1] = this.__id;

    end += 2;


    uint32View[0] = end;

    if (end > flushThres) {
      cb.flush();
    }
  }

  function voidFromString(s) {
    let end = uint32View[0];
    uint32View[end] = 2;
    uint32View[end + 1] = this.__id;

    let sLen = s.length; // throws when null
    if (sLen / 2 > cmdLengthCutoff) {
      this.voidFromString_(...arguments);
      return;
    }
    uint32View[end + 2] = sLen;
    end += 3;

    str2view(s, dataView, end);
    end += Math.ceil(sLen / 2);

    uint32View[0] = end;

    if (end > flushThres) {
      cb.flush();
    }
  }

  function voidFromStringArray(sa) {
    let end = uint32View[0];
    uint32View[end] = 3;
    uint32View[end + 1] = this.__id;

    uint32View[end + 2] = sa.length;
    end += 3;

    for (let s of sa) {
      uint32View[end] = s.length;
      str2view(s, dataView, end + 1);
      end += Math.ceil(s.length / 2) + 1;
    }

    uint32View[0] = end;

    if (end > flushThres) {
      cb.flush();
    }
  }

  function voidFromTypedArray(fa) {
    let end = uint32View[0];
    uint32View[end] = 4;
    uint32View[end + 1] = this.__id;

    let faLen = fa.byteLength / 4; // throws when null
    if (!faLen) {
      // Convert array to typed array.
      fa = arguments[0] = new Float32Array(fa);
    }
    faLen = fa.length;
    if (faLen > cmdLengthCutoff) {
      // Call binding when the data is too large.
      this.voidFromTypedArray_(...arguments);
      return;
    }
    uint32View[end + 2] = faLen;
    end += 3;

    float32View.set(fa, end);
    end += fa.length;

    uint32View[0] = end;

    if (end > flushThres) {
      cb.flush();
    }
  }

  function voidFromArrayBuffer(ab) {
    let end = uint32View[0];
    uint32View[end] = 5;
    uint32View[end + 1] = this.__id;

    let abLen = ab.byteLength / 4; // throws when null
    if (abLen > cmdLengthCutoff) {
      // Call binding when the data is too large.
      this.voidFromArrayBuffer_(...arguments);
      return;
    }
    const abBufferView = new Uint8Array(ab);
    uint32View[end + 2] = abBufferView.byteLength;
    end += 3;

    uint8View.set(abBufferView, end * 4);
    end += Math.ceil(abBufferView.byteLength / 4);

    uint32View[0] = end;

    if (end > flushThres) {
      cb.flush();
    }
  }

  function voidFromArrayBufferView(abv) {
    let end = uint32View[0];
    uint32View[end] = 6;
    uint32View[end + 1] = this.__id;

    let abvLen = abv.byteLength / 4; // throws when null
    if (abvLen > cmdLengthCutoff) {
      // Call binding when the data is too large.
      this.voidFromArrayBufferView_(...arguments);
      return;
    }
    let abvBufferView;
    if (ArrayBuffer.isView(abv)) {
      abvBufferView = new Uint8Array(abv.buffer, abv.byteOffset, abv.byteLength);
      uint32View[end + 2] = getViewType(abv);
    } else {
      abvBufferView = new Uint8Array(abv);
      uint32View[end + 2] = 0;
    }
    uint32View[end + 3] = abvBufferView.byteLength;
    end += 4;

    uint8View.set(abvBufferView, end * 4);
    end += Math.ceil(abvBufferView.byteLength / 4);

    uint32View[0] = end;

    if (end > flushThres) {
      cb.flush();
    }
  }

  function voidFromNullableArrayBufferView(abv) {
    let end = uint32View[0];
    uint32View[end] = 7;
    uint32View[end + 1] = this.__id;

    let abvLen = abv?.byteLength / 4;
    if (abvLen > cmdLengthCutoff) {
      // Call binding when the data is too large.
      this.voidFromNullableArrayBufferView_(...arguments);
      return;
    }
    let abvBufferView;
    if (ArrayBuffer.isView(abv)) {
      abvBufferView = new Uint8Array(abv.buffer, abv.byteOffset, abv.byteLength);
      uint32View[end + 2] = getViewType(abv);
    } else {
      abvBufferView = new Uint8Array(abv);
      uint32View[end + 2] = 0;
    }
    uint32View[end + 3] = abvBufferView.byteLength;
    end += 4;

    uint8View.set(abvBufferView, end * 4);
    end += Math.ceil(abvBufferView.byteLength / 4);

    uint32View[0] = end;

    if (end > flushThres) {
      cb.flush();
    }
  }

  function createAsyncObject() {
    let id = globalAsyncId++;
    let puppet = new TestAsyncObject(id);
    puppet.__id = id;
    objects.push(puppet);
    let end = uint32View[0];
    uint32View[end] = 8;
    uint32View[end + 1] = this.__id;

    uint32View[end + 2] = id;
    end += 3;


    uint32View[0] = end;

    if (end > flushThres) {
      cb.flush();
    }
    return puppet;
  }

  function asyncForAsyncObject(tao) {
    if (tao && !(tao instanceof TestAsyncObject)) {
      throw new TypeError('Invalid type for arg 0, TestAsyncObject expected');
    }
    let end = uint32View[0];
    uint32View[end] = 9;
    uint32View[end + 1] = this.__id;

    uint32View[end + 2] = tao.__id;
    objects.push(tao);
    end += 3;


    uint32View[0] = end;

    if (end > flushThres) {
      cb.flush();
    }
  }

  function asyncForNullableAsyncObject(tao) {
    if (tao && !(tao instanceof TestAsyncObject)) {
      throw new TypeError('Invalid type for arg 0, TestAsyncObject expected');
    }
    let end = uint32View[0];
    uint32View[end] = 10;
    uint32View[end + 1] = this.__id;

    uint32View[end + 2] = tao?.__id;
    objects.push(tao);
    end += 3;


    uint32View[0] = end;

    if (end > flushThres) {
      cb.flush();
    }
  }

  function syncForAsyncObject(tao) {
    if (tao && !(tao instanceof TestAsyncObject)) {
      throw new TypeError('Invalid type for arg 0, TestAsyncObject expected');
    }
    let end = uint32View[0];
    uint32View[end] = 11;
    uint32View[end + 1] = this.__id;

    uint32View[end + 2] = tao.__id;
    end += 3;


    uint32View[0] = end;

    const result = cb.call();
    objects.length = 0;
    return result;
  }

  TestContextCB = {
    voidFromVoid: voidFromVoid,
    voidFromString: voidFromString,
    voidFromStringArray: voidFromStringArray,
    voidFromTypedArray: voidFromTypedArray,
    voidFromArrayBuffer: voidFromArrayBuffer,
    voidFromArrayBufferView: voidFromArrayBufferView,
    voidFromNullableArrayBufferView: voidFromNullableArrayBufferView,
    createAsyncObject: createAsyncObject,
    asyncForAsyncObject: asyncForAsyncObject,
    asyncForNullableAsyncObject: asyncForNullableAsyncObject,
    syncForAsyncObject: syncForAsyncObject,
  };

  return TestContextCB;
};

let protoHooked = false;

function hookTestContext(appendTarget, context) {
  const commandBuffer = commandBufferCreator(appendTarget);
  if (protoHooked) return;
  protoHooked = true;
  let ctxProto = context.__proto__;
  ctxProto.voidFromVoid = commandBuffer.voidFromVoid;
  ctxProto.voidFromString = ctxProto.voidFromString_;
  ctxProto.voidFromString$ = commandBuffer.voidFromString;
  ctxProto.voidFromStringArray = ctxProto.voidFromStringArray_;
  ctxProto.voidFromStringArray$ = commandBuffer.voidFromStringArray;
  ctxProto.voidFromTypedArray = commandBuffer.voidFromTypedArray;
  ctxProto.voidFromArrayBuffer = commandBuffer.voidFromArrayBuffer;
  ctxProto.voidFromArrayBufferView = commandBuffer.voidFromArrayBufferView;
  ctxProto.voidFromNullableArrayBufferView = commandBuffer.voidFromNullableArrayBufferView;
  ctxProto.createAsyncObject = commandBuffer.createAsyncObject;
  ctxProto.asyncForAsyncObject = commandBuffer.asyncForAsyncObject;
  ctxProto.asyncForNullableAsyncObject = commandBuffer.asyncForNullableAsyncObject;
  ctxProto.syncForAsyncObject = commandBuffer.syncForAsyncObject;

}

export { hookTestContext };