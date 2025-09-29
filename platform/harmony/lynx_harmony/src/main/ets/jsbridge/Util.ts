// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export interface LynxModuleConstructor extends Function {
  new (context: Object | undefined, param: Object): Object;
}

export interface SendableLynxModuleConstructor extends Function {
  new (param: Object): Object;
}

export function constructModule(
  cons: Function,
  context: Object | undefined,
  param: Object
): Object {
  return new (cons as LynxModuleConstructor)(context, param);
}

export function constructSendableModule(cons: Function, param: Object): Object {
  return new (cons as SendableLynxModuleConstructor)(param);
}
