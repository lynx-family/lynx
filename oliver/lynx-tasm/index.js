// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

const path = require('path');
const fs = require('fs');

let task = null;
function supportNapi(){
  return process.platform === 'darwin' || (process.platform === 'linux' && process.arch === "x64")
}

function loadModule() {
  // eslint-disable-next-line @typescript-eslint/no-var-requires
  const _loadModule = require('./lepus');
  if (!task) {
    task = _loadModule();
  }
  return task;
}

async function encode_wasm(options) {
  const m = await loadModule();
  /** @type {import('./index').EncodeResult} */
  const res = m._encode(JSON.stringify(options))
  if (res.status !== 0) {
    throw new Error(`encode error: ${res.error_msg}`);
  }
  const uint8array = new Uint8Array(res.buffer.size());
  for (let i = 0; i < res.buffer.size(); i++) {
    uint8array[i] = res.buffer.get(i);
  }
  res.buffer = Buffer.from(uint8array);
  if(options.onTemplateDebugGenerated){
      const url = await options.onTemplateDebugGenerated(res.template_debug, options.templateDebugUrl);
      const template_url_ptr = m.allocateUTF8(url);
      const res_str_ptr = m._reencode_template_debug(bufferPtr, bufferLen, template_url_ptr, buffer_pool);
      const reEncoded_res_str = m.UTF8ToString(res_str_ptr);
      const reEncodedRes = JSON.parse(reEncoded_res_str);
      const reEncodedBufferPtr = m._readBufferPool_data(buffer_pool, reEncodedRes.buffer);
      const reEncodedBufferLen = m._readBufferPool_len(buffer_pool, reEncodedRes.buffer);
      res.buffer = Array.from(new Uint8Array(m.HEAPU8.buffer, reEncodedBufferPtr, reEncodedBufferLen));
  }
  return res;
}
function encode_napi(options) {
  const bindingPath = path.resolve(__dirname, `./build/${process.platform}/Release/lepus.node`);
  const lepus = require(bindingPath);
  const res = lepus.encode(JSON.stringify({...options}));
  if (res.status !== 0) {
    throw new Error(`encode error: ${res.error_msg}`);
  }
  return res
}

function lepusCheck(sourceFile, targetSdkVersion, execute = 0) {
  const m = loadModule();
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
  const bindingPath = path.resolve(__dirname, `./build/${process.platform}/Release/lepus.node`);
  const lepus = require(bindingPath);
  const res = lepus.encrypt(plain);
  return res
}

function decrypt(cipher) {
  const bindingPath = path.resolve(__dirname, `./build/${process.platform}/Release/lepus.node`);
  const lepus = require(bindingPath);
  const res = lepus.decrypt(cipher);
  return res
}

function encrypt_wasm(plain) {
  const m = loadModule();
  const res = m._encrypt(plain)
  return res;
}

function decrypt_wasm(plain) {
  const m = loadModule();
  const res = m._decrypt(plain)
  return res;
}

let encode = encode_napi;
module.exports = {
  supportNapi,
  encode,
  encode_napi,
  encode_wasm,
  lepusCheck,
  loadModule,
  getEncodeMode,
  encrypt,
  decrypt,
  encrypt_wasm,
  decrypt_wasm
};
