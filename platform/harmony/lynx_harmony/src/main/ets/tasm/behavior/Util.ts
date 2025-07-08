// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export interface UICreator extends Function {
  new (context: Object, param?: Object): Object;
}

export interface ShadowNodeCreator extends Function {
  new (context: Object): Object;
}
