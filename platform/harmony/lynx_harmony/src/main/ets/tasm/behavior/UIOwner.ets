// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import lynx, { NativeContent } from 'liblynx.so';
import { UIContext } from '@kit.ArkUI';
import { BehaviorRegistryMap, NodeType } from './Behavior';
import { NodeContent } from '@ohos.arkui.node';
import { TraceCategory, TraceEvent } from '../../base/TraceEvent';
import { TraceEventDef } from '../../base/TraceEventDef';
import { FluencyTraceHelper } from '../performance/fluency/FluencyTraceHelper';
import { LynxContext } from '../LynxContext';
import { DrawEndTimingFrameCallback } from '../performance/DrawEndTimingFrameCallback';
import { PerformanceController } from '../performance/PerformanceController';
import { LynxResourceLoadInfo, LynxViewClient } from '../LynxViewClient';

export class UIOwner {
  native: lynx.UIOwner;
  uiContext: UIContext;
  behaviors: BehaviorRegistryMap;
  private perfController: PerformanceController;
  private readonly id: string;
  protected fluencyTraceHelper?: FluencyTraceHelper;
  private avoidKeyboardListener?: (translateY: number) => void;
  lynxContext: LynxContext;
  private clients?: LynxViewClient[];

  constructor(behaviors: BehaviorRegistryMap, uiContext: UIContext, lynxContext: LynxContext,
    perfCollector: PerformanceController) {
    TraceEvent.beginSection(TraceCategory.Lynx, TraceEventDef.UIOWNER_INIT);
    this.behaviors = behaviors;
    this.perfController = perfCollector;
    this.native = new lynx.UIOwner(this, this.create, uiContext, this.createNativeContent, this.startFluencyTrace,
      this.stopFluencyTrace, this.getNodeType, this.postDrawEndTimingFrameCallback, this.onAvoidKeyboardCallback,
      this.onResourceLoaded);
    this.uiContext = uiContext;
    this.id = this.native.getId();
    this.lynxContext = lynxContext;
    this.clients = this.lynxContext.getLynxViewClient();
    if (this.lynxContext.getLynxImageConfig() !== undefined) {
      this.native.setLynxImageConfig(this.lynxContext.getLynxImageConfig());
    }
    TraceEvent.endSection(TraceCategory.Lynx, TraceEventDef.UIOWNER_INIT);
  }
  
  onResourceLoaded(result: LynxResourceLoadInfo) {
    this.clients?.forEach(client => client.onResourceLoaded(result));
  }

  startFluencyTrace(sign: number, scene: string, tag: string, instanceId: number, probability: number) {
    if (!this.fluencyTraceHelper) {
      this.fluencyTraceHelper = new FluencyTraceHelper();
    }
    this.fluencyTraceHelper.start(sign, scene, tag, instanceId, probability);
  }

  stopFluencyTrace(sign: number) {
    if (!this.fluencyTraceHelper) {
      return;
    }
    this.fluencyTraceHelper.stop(sign);
  }

  postDrawEndTimingFrameCallback(pipelineId:string) {
    this.uiContext?.postFrameCallback(new DrawEndTimingFrameCallback(this.perfController, pipelineId));
  }

  getId(): string {
    return this.id;
  }

  getNodeType(tag: string): NodeType {
    return this.behaviors.get(tag)?.type ?? NodeType.UNKNOWN;
  }

  setEnableSyncXElementRegistry(): void {
    const behavior = this.behaviors.get('x-webview');
    if (behavior !== undefined) {
      this.behaviors.set('webview', behavior);
    }
  }

  attachPageRoot(nativeContent: NativeContent) {
    this.native.attachPageRoot(nativeContent);
  }

  create(context: number[], tag: string, sign: number): lynx.UIBase | undefined {
    if (!this.uiContext) {
      return
    }
    let behavior = this.behaviors.get(tag);
    if (behavior !== undefined) {
      const traceEvent = TraceEventDef.UIOWNER_CREATE_VIEW + tag;
      TraceEvent.beginSection(TraceCategory.Lynx, traceEvent);
      let ui = behavior.createUI(this.lynxContext);
      if (ui !== undefined) {
        ui.setSign(sign, tag);
        ui.attachToNative(this.uiContext, context);
        ui.onCreate();
        TraceEvent.endSection(TraceCategory.Lynx, traceEvent);
        return ui.nativeUI;
      }
      TraceEvent.endSection(TraceCategory.Lynx, traceEvent);
    }
    return;
  }

  createNativeContent(): lynx.NativeContent {
    let content = new NodeContent();
    let nodeContent = new lynx.NativeContent(content);
    return nodeContent;
  }

  destroy() {
    this.setAvoidKeyboardListener(undefined);
    this.native.destroy(this.id);
    if (this.fluencyTraceHelper) {
      this.fluencyTraceHelper.destroy();
    }
  }

  requestLayout() {
    this.native.requestLayout();
  }

  keyboardStatusChanged(height: number) {
    this.native.keyboardStatusChanged(height);
  }

  setAvoidKeyboardListener(avoidKeyboardListener?: (translateY: number) => void):void {
    this.avoidKeyboardListener = avoidKeyboardListener;
  }

  onAvoidKeyboardCallback(translateY: number) {
    this.avoidKeyboardListener?.(translateY)
  }

  canConsumeTouchEvent(x: number, y: number): boolean {
    return this.native.canConsumeTouchEvent(x, y);
  }

  updateRootTarget(content: NativeContent): void {
    this.native.updateRootTarget(content);
  }

  public onEnterForeground(): void {
    this.native.onEnterForeground();
  }

  public onEnterBackground(): void {
    this.native.onEnterBackground();
  }
}
