// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import lynx from 'liblynx.so';
import { BuilderNode, display, FrameNode, UIContext } from '@kit.ArkUI';
import { IContext } from '../../IContext';
import { EventHandler, EventHandlerArray } from './EventHandler';
import { TraceCategory, TraceEvent } from '../../../base/TraceEvent';
import { TraceEventDef } from '../../../base/TraceEventDef';

export abstract class UIBase {
  uiContext?: UIContext;
  nativeUI?: lynx.UIBase;
  lynxContext: IContext;
  sign!: number;
  tag!: string;
  density?: number;
  width: number = 0;
  height: number = 0;
  x: number = 0;
  y: number = 0;
  eventMap?: Map<string, EventHandler>;
  builder?: WrappedBuilder<[UIBase]>;
  builderNode?: BuilderNode<[UIBase]>;
  logicalPixels: boolean = false;
  protected param?: Object;

  constructor(context: IContext, param?: Object) {
    this.lynxContext = context;
    this.param = param;
  }

  onCreate() {
  }

  onEnterForeground() {
  }

  onEnterBackground() {
  }

  setSign(sign: number, tag: string) {
    this.sign = sign;
    this.tag = tag;
  }


  hasCustomizedLayout(): boolean {
    return false;
  }

  focusable(): boolean {
    return false;
  }

  updateExtraData(data: Object): void {
  }

  private getFrameNode(uiContext: UIContext): FrameNode | null {
    if (this.builder) {
      if (!this.builderNode) {
        this.builderNode = new BuilderNode<[UIBase]>(uiContext);
      }
      this.builderNode.build(this.builder, this);
      return this.builderNode.getFrameNode();
    }
    return null;
  }

  protected needWindowStateChangeEvent() {
    return false;
  }

  attachToNative(uiContext: UIContext, context: number[]): void {
    this.logicalPixels = true;
    this.uiContext = uiContext;
    TraceEvent.beginSection(TraceCategory.Lynx, TraceEventDef.UIBASE_CREATE_NATIVE_UI);
    this.nativeUI =
      new lynx.UIBase(this, context, null, this.sign!, this.tag!, this.update, this.layout, this.invokeMethod,
        this.dispose,
        this.focusChange, this.focusable, this.onNodeReady, this.hasCustomizedLayout(), this.updateExtraData,
        this.needWindowStateChangeEvent(),
        this.onEnterForeground, this.onEnterBackground);
    TraceEvent.endSection(TraceCategory.Lynx, TraceEventDef.UIBASE_CREATE_NATIVE_UI);
    this.nativeUI.setFrameNode(this.getFrameNode(this.uiContext!));
  }

  sendGlobalEvent(name: string, params: Object[]) {
    this.lynxContext.sendGlobalEvent(name, params);
  }

  sendCustomEvent(name: string, params: Object, paramName?: string) {
    this.lynxContext.sendCustomEvent(name, this.sign!, params, paramName ? paramName : 'params');
  }

  update(props: Object, events?: EventHandlerArray[]): void {
    if (events !== undefined) {
      if (!this.eventMap) {
        this.eventMap = new Map<string, EventHandler>();
      }
      for (let e of events) {
        this.eventMap.set(e[0] as string, {
          name: e[0] as string,
          type: e[1] as string,
          jsEvent: e[2] as boolean,
          function: e[3] as string
        });
      }
    }
  }

  /**
   *
   * @param method ui methods name
   * @param params methods params
   * @param callback callback for this method, which needs a method return type and data for methods result.

   Code for the callback only accept one of LynxUIMethodConstants.
   0 - SUCCESS
   1 - UNKNOWN
   2 - NODE_NOT_FOUND
   3 - METHOD_NOT_FOUND
   4 - PARAM_INVALID
   5 - SELECTOR_NOT_SUPPORTED
   6 - NO_UI_FOR_NODE
   7 - INVALID_STATE_ERROR
   8 - OPERATION_ERROR

   * @returns true - If the method with the name is to be executed.
   *          false - No target method implemented. Don't execute the method if you are not going to respond the ui method.
   */
  invokeMethod(method: string, params: Object, callback: (code: number, res: Object) => void): boolean {
    return false;
  }

  dispose() {
    this.builderNode?.update(undefined);
    this.builderNode?.dispose();
    this.nativeUI = undefined;
  }

  focusChange(focus: boolean, isFocusTransition: boolean) {
  }

  onNodeReady() {
  }

  setFocusedUI() {
    this.nativeUI?.setFocusedUI();
  }

  unsetFocusedUI() {
    this.nativeUI?.unsetFocusedUI();
  }

  px2vpIfNeeded(px: number, defaultValue: number = 0): number {
    if (typeof px != 'number') {
      return defaultValue;
    }
    if (this.logicalPixels) {
      return px;
    }
    return px2vp(px);
  }

  value2vpIfNeeded(value: number | string, defaultValue: number = 0): number {
    if (typeof value == 'number') {
      return value;
    } else if (typeof value == 'string') {
      if (value.endsWith('rpx')) {
        let density: number = this.lynxContext.getDensity();
        return Number.parseFloat(value.substring(0, value.length - 3)) * display.getDefaultDisplaySync().width /
          density / 750.0;
      } else if (value.endsWith('ppx')) {
        let density: number = this.lynxContext.getDensity();
        return Number.parseFloat(value.substring(0, value.length - 3)) / density;
      } else if (value.endsWith('px')) {
        return Number.parseFloat(value.substring(0, value.length - 2));
      } else {
        return Number.parseFloat(value);
      }
    }
    return defaultValue
  }

  abstract layout(x: number, y: number, width: number, height: number, paddingLeft: number,
    paddingTop: number, paddingRight: number, paddingBottom: number, marginLeft: number, marginTop: number,
    marginRight: number, marginBottom: number, sticky?: number[]): void;

  static getUIFromNativeContent(nativeContent: lynx.NativeContent) {
    return lynx.UIBase.getUIFromNativeContent(nativeContent);
  }
}
