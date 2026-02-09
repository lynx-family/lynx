// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import lazy { SvgData, SvgRenderNodeController } from '@lynx/servalsvg';
import { LLog, LynxResourceRequest, LynxResourceType, UIBase, EventHandlerArray } from '@lynx/lynx';
import { BusinessError } from '@ohos.base';
import { util } from '@kit.ArkTS';

interface SvgLoadEvent {
  width: number,
  height: number,
}

export class UISVG extends UIBase {
  private content: string = '';
  private needUpdateContent = false;
  private needRequestSrc = false;
  private svgNodeController: SvgRenderNodeController = new SvgRenderNodeController();
  private src: string | undefined = undefined;
  private allowAntiAlias: boolean = true;
  private sSvgBase64Scheme:string = "data:image/svg+xml;base64";

  layout(x: number, y: number, width: number, height: number, paddingLeft: number, paddingTop: number,
    paddingRight: number, paddingBottom: number, marginLeft: number, marginTop: number, marginRight: number,
    marginBottom: number, sticky?: number[] | undefined): void {
    if (this.width !== width) {
      this.width = width;
      this.needUpdateContent = true;
    }
    if (this.height !== height) {
      this.height = height;
      this.needUpdateContent = true;
    }
  }

  override onCreate() {
    this.nativeUI?.setFrameNode(this.svgNodeController.makeNode(this.uiContext)!);
  }

  update(prop: Record<string, Object>, events?: EventHandlerArray[]): void {
    if (prop.content !== undefined && prop.content !== this.content) {
      this.content = prop.content as string;
      this.needUpdateContent = true;
    } else if (prop.src !== undefined && this.src !== prop.src) {
      this.src = prop.src as string;
      this.needRequestSrc = true;
    }
    if (prop['allow-edge-antialiasing'] !== undefined) {
      this.allowAntiAlias = prop['allow-edge-antialiasing'] as boolean;
    }
  }

  onNodeReady(): void {
    const density = this.lynxContext?.getDensity();
    if (this.density !== density) {
      this.density = density;
      this.needUpdateContent = true;
    }
    if (this.needUpdateContent) {
      const roundWidth = Math.round(this.density! * this.width) / this.density!
      const roundHeight = Math.round(this.density! * this.height) / this.density!
      const data: SvgData = {
        width: roundWidth,
        height: roundHeight,
        density: this.density,
        content: this.content,
        antiAlias: this.allowAntiAlias,
      }
      this.svgNodeController.updateData(data);
      this.sendCustomEvent('load', { width: roundWidth, height: roundHeight } as SvgLoadEvent, 'detail');
      this.needUpdateContent = false;
    }
    if (this.needRequestSrc) {
       // base 64
      if (this.src!.startsWith(this.sSvgBase64Scheme)) {
        const base64Str = this.src!.substring(this.sSvgBase64Scheme.length + 1);
        const base64Helper = new util.Base64Helper();
        const uint8Array = base64Helper.decodeSync(base64Str);
        const decoder = util.TextDecoder.create('utf-8', { ignoreBOM: true });
        this.content = decoder.decode(uint8Array);
        const roundWidth = Math.round(this.density! * this.width) / this.density!
        const roundHeight = Math.round(this.density! * this.height) / this.density!
        const data: SvgData = {
          width: roundWidth,
          height: roundHeight,
          density: this.density,
          content: this.content,
          antiAlias: this.allowAntiAlias,
        }
        this.svgNodeController.updateData(data);
        this.sendCustomEvent('load', { width: roundWidth, height: roundHeight } as SvgLoadEvent, 'detail');
        this.needRequestSrc = false;
        return;
      }
      
      // fetch src
      const request = new LynxResourceRequest(this.src!, LynxResourceType.LYNX_RESOURCE_TYPE_SVG);
      this.lynxContext?.getGenericResourceFetcher()?.fetchResource(request,
        (err: BusinessError, res: ArrayBuffer) => {
          if (err) {
            LLog.e('UISvg', `SVG getSrcResource Failed code = ${err.code} message = ${err.message}`);
          } else {
            const decoder = util.TextDecoder.create('utf-8', { ignoreBOM: true });
            this.content = decoder.decode(new Uint8Array(res));
            const data: SvgData = {
              width: this.width,
              height: this.height,
              density: this.density,
              content: this.content,
              antiAlias: this.allowAntiAlias,
            }
            this.svgNodeController.updateData(data);
            this.sendCustomEvent('load', { width: this.width, height: this.height } as SvgLoadEvent, 'detail');
          }
        });
      this.needRequestSrc = false;
    }
  }
}
