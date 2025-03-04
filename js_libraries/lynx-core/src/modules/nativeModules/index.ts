// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export * from './textInfo';
export * from './exposure';
export * from './intersectionObserver';

export interface NativeModule {
  LynxTestModule;
  LynxUIMethodModule;
  LynxResourceModule;
  LynxAccessibilityModule;
  LynxFetchModule;
  LynxSetModule;
}

export type CallLynxSetModule = (functionName: string, args: any[]) => void;

export interface NativeLynxUIModule {
  invokeUIMethod: Function;
  invokeUIMethodForSelectorQuery: Function;
}
