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

@Builder
function buildFrameHost(host: FrameHost) {
  Stack() {
    LynxView(host.createLynxViewBuilder())
      .width(host.autoWidth ? 'auto' : '100%')
      .height(host.autoHeight ? 'auto' : '100%')
      .onAreaChange((oldValue: Area, newValue: Area) => {
        host.onChildAreaChanged(areaSize(newValue.width),
          areaSize(newValue.height));
      })
  }
  .width(host.autoWidth ? 'auto' : '100%')
  .height(host.autoHeight ? 'auto' : '100%')
}

export class FrameHost {
  readonly parentContext: LynxContext;
  autoWidth: boolean = false;
  autoHeight: boolean = false;
  private readonly native: lynx.UIFrame;
  private readonly uiContext: UIContext;
  private builderNode?: BuilderNode<[FrameHost]>;
  private childContext?: LynxContext;

  constructor(parentContext: LynxContext, uiContext: UIContext, nativeFrame: lynx.UIFrame) {
    this.parentContext = parentContext;
    this.uiContext = uiContext;
    this.native = nativeFrame;
    this.builderNode = new BuilderNode<[FrameHost]>(this.uiContext);
    this.builderNode.build(wrapBuilder<[FrameHost]>(buildFrameHost), this);
  }

  getFrameNode(): FrameNode | null {
    return this.builderNode?.getFrameNode() ?? null;
  }

  createLynxViewBuilder(): LynxViewBuilder {
    const builder = this.parentContext.createChildLynxViewBuilder({
      fitContentWidth: this.autoWidth,
      fitContentHeight: this.autoHeight,
    });
    const parentOnCreate = builder.onCreate;
    builder.onCreate = (context: LynxContext) => {
      parentOnCreate?.(context);
      this.onChildContextReady(context);
    };
    return builder;
  }

  updateAutoSize(autoWidth: boolean, autoHeight: boolean): void {
    if (this.autoWidth === autoWidth && this.autoHeight === autoHeight) {
      return;
    }
    this.autoWidth = autoWidth;
    this.autoHeight = autoHeight;
    this.builderNode?.update(this);
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

  onEnterForeground(): void {
    this.childContext?.onEnterForeground();
  }

  onEnterBackground(): void {
    this.childContext?.onEnterBackground();
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
