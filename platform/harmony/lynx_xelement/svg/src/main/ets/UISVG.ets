// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import lazy { SvgData, SvgRenderNodeController } from '@lynx/servalsvg';
import { LLog, LynxResourceRequest, LynxResourceType, UIBase, EventHandlerArray } from '@lynx/lynx';
import { BusinessError } from '@ohos.base';
import { util } from '@kit.ArkTS';
import { image } from '@kit.ImageKit';

interface SvgLoadEvent {
  width: number,
  height: number,
}

export class UISVG extends UIBase {
  private content: string = '';
  private color?: string = undefined;
  private needUpdateContent = false;
  private needRequestSrc = false;
  private svgNodeController: SvgRenderNodeController = new SvgRenderNodeController();
  private src: string | undefined = undefined;
  private allowAntiAlias: boolean = true;
  private svgBase64Scheme: string = 'data:image/svg+xml;base64';

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
    this.svgNodeController.setImageFetcher(this.fetchSvgImage.bind(this));
    this.nativeUI?.setFrameNode(this.svgNodeController.makeNode(this.uiContext)!);
  }

  override dispose() {
    this.svgNodeController.dispose();
    super.dispose();
  }

  update(prop: Record<string, Object>, events?: EventHandlerArray[]): void {
    if (prop.content !== undefined && prop.content !== this.content) {
      this.content = prop.content as string;
      this.needUpdateContent = true;
    } else if (prop.src !== undefined && this.src !== prop.src) {
      this.src = prop.src as string;
      this.needRequestSrc = true;
    }
    const propColor = prop['current-color'];
    if (propColor !== undefined) {
      const nextColor = typeof propColor === 'string' && propColor.length > 0 ? propColor : undefined;
      if (nextColor !== this.color) {
        this.color = nextColor;
        this.needUpdateContent = true;
      }
    }
    if (prop['allow-edge-antialiasing'] !== undefined) {
      this.allowAntiAlias = prop['allow-edge-antialiasing'] as boolean;
    }
  }

  private renderContent(shouldEmitLoad: boolean): void {
    const roundWidth = Math.round(this.density! * this.width) / this.density!;
    const roundHeight = Math.round(this.density! * this.height) / this.density!;
    const data: SvgData = {
      width: roundWidth,
      height: roundHeight,
      density: this.density,
      content: this.content,
      antiAlias: this.allowAntiAlias,
    }
    this.svgNodeController.updateData(data, this.color);
    if (shouldEmitLoad) {
      this.sendCustomEvent('load', { width: roundWidth, height: roundHeight } as SvgLoadEvent, 'detail');
    }
  }

  private async fetchSvgImage(url: string): Promise<image.PixelMap | undefined> {
    const fetcher = this.lynxContext?.getGenericResourceFetcher();
    if (fetcher === undefined) {
      LLog.e('UISvg', `SVG image request skipped without generic resource fetcher: ${url}`);
      return undefined;
    }
    const data = await new Promise<ArrayBuffer | undefined>((resolve) => {
      const request = new LynxResourceRequest(url, LynxResourceType.LYNX_RESOURCE_TYPE_IMAGE);
      fetcher.fetchResource(request, (err: BusinessError, res: ArrayBuffer) => {
        if (err) {
          LLog.e('UISvg', `SVG getImageResource Failed code = ${err.code} message = ${err.message}`);
          resolve(undefined);
          return;
        }
        if (res === undefined || res.byteLength === 0) {
          LLog.e('UISvg', `SVG getImageResource empty data url = ${url}`);
          resolve(undefined);
          return;
        }
        resolve(res);
      });
    });
    if (data === undefined) {
      return undefined;
    }
    const imageSource = image.createImageSource(data);
    try {
      return await imageSource.createPixelMap();
    } catch (err) {
      const error = err as BusinessError;
      LLog.e('UISvg', `SVG decode image failed code = ${error.code} message = ${error.message}`);
      return undefined;
    } finally {
      try {
        await imageSource.release();
      } catch (err) {
        const error = err as BusinessError;
        LLog.e('UISvg', `SVG release image source failed code = ${error.code} message = ${error.message}`);
      }
    }
  }

  onNodeReady(): void {
    const density = this.lynxContext?.getDensity();
    if (this.density !== density) {
      this.density = density;
      this.needUpdateContent = true;
    }
    if (this.needUpdateContent) {
      this.renderContent(true);
      this.needUpdateContent = false;
    }
    if (this.needRequestSrc) {
      // base64
      if (this.src!.startsWith(this.svgBase64Scheme)) {
        const base64Str = this.src!.substring(this.svgBase64Scheme.length + 1);
        const base64Helper = new util.Base64Helper();
        const uint8Array = base64Helper.decodeSync(base64Str);
        const decoder = util.TextDecoder.create('utf-8', { ignoreBOM: true });
        this.content = decoder.decode(uint8Array);
        this.renderContent(true);
        this.needRequestSrc = false;
        return;
      }

      const request = new LynxResourceRequest(this.src!, LynxResourceType.LYNX_RESOURCE_TYPE_SVG);
      this.lynxContext?.getGenericResourceFetcher()?.fetchResource(request,
        (err: BusinessError, res: ArrayBuffer) => {
          if (err) {
            LLog.e('UISvg', `SVG getSrcResource Failed code = ${err.code} message = ${err.message}`);
          } else {
            const decoder = util.TextDecoder.create('utf-8', { ignoreBOM: true });
            this.content = decoder.decode(new Uint8Array(res));
            this.renderContent(true);
          }
        });
      this.needRequestSrc = false;
    }
  }
}
