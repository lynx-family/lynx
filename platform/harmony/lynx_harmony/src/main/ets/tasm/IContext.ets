// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
import { LynxGenericResourceFetcher, LynxMediaResourceFetcher } from '../Index';

export interface IContext {
  setContextData(data?: Record<string, Object>): void;

  getContextData(): Record<string, Object> | undefined;

  sendCustomEvent(name: string, id: number, params: Object, paramName?: string): void;

  findJSShadowNodeBySign(sign: number): Object | undefined;

  sendGlobalEvent(name: string, params: Object[]): void;

  measureNativeNode(sign: number,
    width: number,
    widthMode: number,
    height: number,
    heightMode: number,
    finalMeasure: boolean): [number, number, number] | undefined;

  getGenericResourceFetcher(): LynxGenericResourceFetcher | undefined;

  getMediaResourceFetcher(): LynxMediaResourceFetcher | undefined;

  getDensity(): number;
}
