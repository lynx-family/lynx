// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import loadMappingsWasm from './initSourcemapConsumer';

let mappingsWasm: ArrayBuffer = new ArrayBuffer(0);

const loadFile = (filePayload: any) => {
  const { type, data } = filePayload;
  if (typeof type !== 'string' || typeof data !== 'string' || data.length === 0) {
    console.warn('failed to load file: invalid file data');
  }
  const decodedData = Buffer.from(data, 'base64');
  const unit8Array = Uint8Array.from(decodedData);
  if (type === 'mappings.wasm') {
    loadMappingsWasm(unit8Array.buffer);
  }
};

const getMappingsWasm = () => {
  return mappingsWasm;
};

export { loadFile, getMappingsWasm };
