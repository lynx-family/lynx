// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

const path = require('path');
const fs = require('fs');

function supportNapi(){
  return process.platform === 'darwin' || (process.platform === 'linux' && process.arch === "x64")
}

function createModule() {
  // Reusing a single WASM module instance makes repeated encode/decode calls
  // on JsBytecode custom sections nondeterministic. Use a fresh instance for
  // codec operations that need stable binary output.
  // eslint-disable-next-line @typescript-eslint/no-var-requires
  return require('./lepus')();
}

function disposeClassHandle(handle) {
  if (!handle || typeof handle.delete !== 'function') {
    return;
  }
  if (typeof handle.isDeleted === 'function' && handle.isDeleted()) {
    return;
  }
  handle.delete();
}

async function encode_wasm(options) {
  const m = await createModule();
  /** @type {import('./index').EncodeResult} */
  const res = m._encode(JSON.stringify(options))
  const bufferHandle = res.buffer;
  try {
    if (res.status !== 0) {
      throw new Error(`encode error: ${res.error_msg}`);
    }
    const out = {
      status: res.status,
      error_msg: res.error_msg,
      lepus_code: res.lepus_code,
      lepus_debug: res.lepus_debug,
      section_size: res.section_size,
    };
    const uint8array = new Uint8Array(bufferHandle.size());
    for (let i = 0; i < bufferHandle.size(); i++) {
      uint8array[i] = bufferHandle.get(i);
    }
    out.buffer = Buffer.from(uint8array);
    if(options.onTemplateDebugGenerated){
      const url = await options.onTemplateDebugGenerated(res.template_debug, options.templateDebugUrl);
      const template_url_ptr = m.allocateUTF8(url);
      const res_str_ptr = m._reencode_template_debug(bufferPtr, bufferLen, template_url_ptr, buffer_pool);
      const reEncoded_res_str = m.UTF8ToString(res_str_ptr);
      const reEncodedRes = JSON.parse(reEncoded_res_str);
      const reEncodedBufferPtr = m._readBufferPool_data(buffer_pool, reEncodedRes.buffer);
      const reEncodedBufferLen = m._readBufferPool_len(buffer_pool, reEncodedRes.buffer);
      out.buffer = Array.from(new Uint8Array(m.HEAPU8.buffer, reEncodedBufferPtr, reEncodedBufferLen));
    }
    return out;
  } finally {
    disposeClassHandle(bufferHandle);
    disposeClassHandle(res);
  }
}
function encode_napi(options) {
  const lepus = require(`./build/${process.platform}/Release/lepus.node`);
  const res = lepus.encode(JSON.stringify({...options}));
  if (res.status !== 0) {
    throw new Error(`encode error: ${res.error_msg}`);
  }
  return res
}

async function lepusCheck(sourceFile, targetSdkVersion, execute = 0) {
  const m = await createModule();
  const _encode = m.cwrap('lepusCheck', 'string', ['string', 'string', 'number']);
  const res = _encode(sourceFile, targetSdkVersion, execute);
  return JSON.parse(res);
}

function getEncodeMode(os) {
  if(os){
    return os === 'napi' ? encode_napi : encode_wasm;
  }else {
    return supportNapi() ? encode_napi : encode_wasm;
  }
}

function encrypt(plain) {
  const lepus = require(`./build/${process.platform}/Release/lepus.node`);
  const res = lepus.encrypt(plain);
  return res
}

function decrypt(cipher) {
  const lepus = require(`./build/${process.platform}/Release/lepus.node`);
  const res = lepus.decrypt(cipher);
  return res
}

async function encrypt_wasm(plain) {
  const m = await createModule();
  if (typeof m._encrypt !== 'function') {
    throw new Error('encrypt_wasm is not supported by the current wasm build');
  }
  const res = m._encrypt(plain)
  return res;
}

async function decrypt_wasm(plain) {
  const m = await createModule();
  if (typeof m._decrypt !== 'function') {
    throw new Error('decrypt_wasm is not supported by the current wasm build');
  }
  const res = m._decrypt(plain)
  return res;
}

function decode_napi(templateJS) {
  const templateArray = Uint8Array.from(templateJS);
  const lepus = require(`./build/${process.platform}/Release/lepus.node`);
  const res = lepus.decode(templateArray);
  if (res.status !== 0) {
    throw new Error(`decode error: ${res.error_msg}`);
  }
  return JSON.parse(res.result);
}

async function decode_wasm(buffer) {
  const Module = await createModule();
  // console.log(templateJs);
  // const uint8array = new Uint8Array(templateJs.size());
  // const res = m._decode(uint8array);
  const byteArray = new Uint8Array(buffer);
  const byteArrayLength = byteArray.length;
  // Allocate memory in the Emscripten heap
  const byteArrayPtr = Module._malloc(byteArrayLength);
  // Copy the data to the Emscripten heap
  Module.HEAPU8.set(byteArray, byteArrayPtr);
  // Call the C++ function
  const res = Module._decode(byteArrayPtr, byteArrayLength);
  // Free the allocated memory
  Module._free(byteArrayPtr);
  try {
    if (res.status !== 0) {
      throw new Error(`decode error: ${res.result}`);
    }
    return JSON.parse(res.result);
  } finally {
    disposeClassHandle(res);
  }
}

let encode = encode_napi;
module.exports = {
  supportNapi,
  encode,
  encode_napi,
  encode_wasm,
  lepusCheck,
  getEncodeMode,
  encrypt,
  decrypt,
  encrypt_wasm,
  decrypt_wasm,
  decode_napi,
  decode_wasm
};
