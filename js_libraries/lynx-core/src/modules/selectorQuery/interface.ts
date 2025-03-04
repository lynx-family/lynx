// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { NativeApp } from '../../app';

export const enum IdentifierType {
  ID_SELECTOR, // css selector
  REF_ID, // for react ref
  UNIQUE_ID, // element_id
}

/**
 * const enum will be shakedown in Typescript Compiler
 */
export const enum ErrorCode {
  SUCCESS = 0,
  UNKNOWN = 1,
  NODE_NOT_FOUND = 2,
  METHOD_NOT_FOUND = 3,
  PARAM_INVALID = 4,
  SELECTOR_NOT_SUPPORTED = 5,
  NO_UI_FOR_NODE = 6,
}

export interface NodeSelectToken {
  type: IdentifierType;
  identifier: string;
  first_only: boolean;

  // root component to search in.
  component_id: string;
  // this is actually element_id of a node and therefore allows user to specify search root.
  // `root_unique_id` has a higher priority than `component_id`.
  // Thus, if `root_unique_id` is not undefined, `component_id` will be ignored.
  root_unique_id: number | undefined;
}

export interface SelectorQueryNativeProxy {
  nativeApp: NativeApp;
  lynx: any;
}
