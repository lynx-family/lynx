// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import lynx from 'liblynx.so';
import { BuilderNode, FrameNode, UIContext } from '@kit.ArkUI';
import { LynxContext } from '../../LynxContext';
import { LynxView, LynxViewBuilder } from '../../LynxView';
import { LynxUpdateMeta, MetaData, TemplateData } from '../../MetaData';
import { TemplateBundle } from '../../TemplateBundle';

type MetaInput = Record<string, Object> | Object;

function createTemplateData(value: MetaInput | undefined): TemplateData | undefined {
  return value === undefined || value === null ? undefined : new TemplateData(value);
}

function areaSize(value: ESObject): number {
  const size = Number(value);
  return Number.isFinite(size) ? Math.max(0, size) : 0;
}

class FrameHostBuildParams {
  host: FrameHost;
  buildChild: boolean;
  autoWidth: boolean;
  autoHeight: boolean;

  constructor(host: FrameHost, buildChild: boolean, autoWidth: boolean, autoHeight: boolean) {
    this.host = host;
    this.buildChild = buildChild;
    this.autoWidth = autoWidth;
    this.autoHeight = autoHeight;
  }
}

@Builder
function buildFrameHost(params: FrameHostBuildParams) {
  Stack() {
    if (params.buildChild) {
      LynxView(params.host.createLynxViewBuilder(params.autoWidth, params.autoHeight))
        .width('100%')
        .height('100%')
        .onAreaChange((oldValue: Area, newValue: Area) => {
          params.host.onChildAreaChanged(areaSize(newValue.width),
            areaSize(newValue.height));
        })
    }
  }
  .width(params.autoWidth ? 'auto' : '100%')
  .height(params.autoHeight ? 'auto' : '100%')
}

export class FrameHost {
  readonly parentContext: LynxContext;
  autoWidth: boolean = false;
  autoHeight: boolean = false;
  private readonly native: lynx.UIFrame;
  private readonly uiContext: UIContext;
  private builderNode?: BuilderNode<[FrameHostBuildParams]>;
  private childContext?: LynxContext;
  private childBuilt: boolean = false;

  constructor(parentContext: LynxContext, uiContext: UIContext, nativeFrame: lynx.UIFrame) {
    this.parentContext = parentContext;
    this.uiContext = uiContext;
    this.native = nativeFrame;
    this.builderNode = new BuilderNode<[FrameHostBuildParams]>(this.uiContext);
    this.builderNode.build(wrapBuilder<[FrameHostBuildParams]>(buildFrameHost),
      new FrameHostBuildParams(this, false, false, false));
  }

  getFrameNode(): FrameNode | null {
    return this.builderNode?.getFrameNode() ?? null;
  }

  createLynxViewBuilder(autoWidth: boolean, autoHeight: boolean): LynxViewBuilder {
    const builder = this.parentContext.getLynxViewBuilder();
    builder.fitContentWidth = autoWidth;
    builder.fitContentHeight = autoHeight;
    // The child frame owns an independent runtime, client list, and lifecycle.
    // Do not inherit the host's single-runtime instance (it would be rebound
    // away from the host), its client list (host observers must not receive the
    // frame page's events), or its onCreate (it belongs to the host context).
    builder.backgroundRuntime = undefined;
    builder.clients = [];
    builder.onCreate = (context: LynxContext) => {
      this.onChildContextReady(context);
    };
    builder.onReuse = (context: LynxContext) => {
      return false;
    };
    return builder;
  }

  updateAutoSize(autoWidth: boolean, autoHeight: boolean): void {
    const changed = this.autoWidth !== autoWidth || this.autoHeight !== autoHeight;
    const needsBuild = !this.childBuilt;
    if (!changed && !needsBuild) {
      return;
    }
    this.autoWidth = autoWidth;
    this.autoHeight = autoHeight;
    this.childBuilt = true;
    this.builderNode?.update(new FrameHostBuildParams(this, true, autoWidth, autoHeight));
  }

  loadTemplate(src: string, bundle: TemplateBundle, data?: MetaInput, globalProps?: MetaInput): void {
    if (!this.childContext || !src || !bundle) {
      return;
    }
    this.childContext.loadTemplate(src, bundle, undefined,
      new MetaData(createTemplateData(data), createTemplateData(globalProps)));
  }

  updateMetaData(data?: MetaInput, globalProps?: MetaInput): void {
    this.childContext?.updateMetaData(new LynxUpdateMeta(
      createTemplateData(data), createTemplateData(globalProps)));
  }

  updateViewport(width: number, widthMode: number, height: number, heightMode: number): void {
    this.childContext?.updateViewport(width, widthMode, height, heightMode);
  }

  onChildContextReady(context: LynxContext): void {
    this.childContext = context;
    this.native.onHostReady();
  }

  onChildAreaChanged(width: number, height: number): void {
    this.native.onIntrinsicSizeChanged(width, height);
  }

  dispose(): void {
    this.childContext = undefined;
    this.builderNode?.dispose();
    this.builderNode = undefined;
  }
}
