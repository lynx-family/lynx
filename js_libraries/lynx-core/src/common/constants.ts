// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export const DEFAULT_ENTRY = '__Card__';
export const APP_SERVICE_NAME = 'app-service.js';
export const SOURCE_MAP_RELEASE_ERROR_NAME = 'LynxGetSourceMapReleaseError';
export interface RUN_TYPE {
  filename: string;
  /** Replace the code with the corresponding commitHash after compilation */
  slot: string;

  /** sourcemap release for kernel */
  release: string;
}

export const LYNX_CORE: RUN_TYPE = {
  filename: 'lynx_core',
  slot: __COMMIT_HASH__,
  release: __BUILD_VERSION__,
};
