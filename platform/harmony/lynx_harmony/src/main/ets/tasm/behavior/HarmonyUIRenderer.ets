// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LLog } from '../../base/LLog';
import { LynxContext } from '../LynxContext';
import { DevtoolResult, EmbedderPlatform } from 'liblynx.so';
import { BehaviorRegistryMap, BUILTIN_BEHAVIORS } from './Behavior';
import { componentSnapshot, Size, UIContext } from '@kit.ArkUI';
import image from '@ohos.multimedia.image';
import { AsyncCallback, BusinessError } from '@ohos.base';
import { TraceCategory, TraceEvent } from '../../base/TraceEvent';
import { TraceEventDef } from '../../base/TraceEventDef';
import { LynxUIRenderer } from '../LynxUIRenderer';
import { PerformanceController } from '../performance/PerformanceController';
import { UIOwner } from './UIOwner';
import { ShadowNodeOwner } from './ShadowNodeOwner';

const TAG = 'HarmonyUIRenderer';

export class HarmonyUIRenderer implements LynxUIRenderer {
  private embedderPlatform: EmbedderPlatform;
  uiOwner: UIOwner;
  private shadowNodeOwner: ShadowNodeOwner;

  constructor(context: LynxContext,
    rootSize: Size,
    behaviors: BehaviorRegistryMap, uiContext: UIContext,
    perfController: PerformanceController) {
    TraceEvent.beginSection(TraceCategory.Lynx, TraceEventDef.TEMPLATE_RENDER_INIT);

    BUILTIN_BEHAVIORS.forEach((value, key) => {
      behaviors.set(key, value);
    });
    // todo(renzhongyue): context here and contextDelegate in UIBase should be merged, they do the same thing.
    this.uiOwner = new UIOwner(behaviors, uiContext, context, perfController);
    context.setUIOwner(this.uiOwner);
    this.shadowNodeOwner = new ShadowNodeOwner(context, behaviors, rootSize);

    TraceEvent.beginSection(TraceCategory.Lynx, TraceEventDef.TEMPLATE_RENDER_CREATE_TASM);
    this.embedderPlatform =
      new EmbedderPlatform(this,
        this.uiOwner.native,
        this.shadowNodeOwner.native);

    TraceEvent.endSection(TraceCategory.Lynx, TraceEventDef.TEMPLATE_RENDER_CREATE_TASM);
    LLog.d(TAG, 'Embedder constructor end');

    TraceEvent.endSection(TraceCategory.Lynx, TraceEventDef.TEMPLATE_RENDER_INIT);
  }

  updateViewport(width: number, height: number): void {
  }

  getUIDelegate(): number[] {
    return this.embedderPlatform.getUIDelegate();
  }

  aboutToDisappear() {
    // unbind the js object of shadowNodeOwner and uiOwner with their native instance first.
    this.shadowNodeOwner.destroy();
    this.uiOwner.destroy();
    // destroy the native instances on their own thread.
    this.embedderPlatform.destroy();
  }

  updateRefreshRate(refreshRate: number) {
    EmbedderPlatform.updateRefreshRate(refreshRate);
  }

  async getScreenShot(quality: number, maxWidth: number, maxHeight: number) {
    let pixelMap: image.PixelMap | undefined;
    let imagePackerApi: image.ImagePacker | undefined;
    try {
      pixelMap = await componentSnapshot.get(this.uiOwner.getId());
      imagePackerApi = image.createImagePacker();
      const imageInfo = pixelMap.getImageInfoSync();
      const imageWidth = imageInfo.size.width;
      const imageHeight = imageInfo.size.height;
      let scale = 1;
      if ((maxHeight !== 0 && maxWidth !== 0) && (imageWidth > maxWidth || imageHeight > maxHeight)) {
        scale = Math.min(maxWidth / imageWidth, maxHeight / imageHeight);
      }
      if (scale !== 1) {
        pixelMap.scaleSync(scale, scale);
      }
      const packOpts: image.PackingOption = { format: "image/png", quality: quality };
      const data = await imagePackerApi.packing(pixelMap, packOpts);
      const response: DevtoolResult = {
        result: data,
        width: imageWidth,
        height: imageHeight,
      }
      return response;
    } catch (e) {
      const errorMsg = "Devtool takeScreenShot packing fail ";
      LLog.d(TAG, errorMsg + JSON.stringify(e));
      throw new Error(errorMsg);
    } finally {
      pixelMap?.release();
      imagePackerApi?.release();
    }
  }

  // Called by native
  takeScreenShot(callback: AsyncCallback<DevtoolResult>, quality: number, maxWidth: number, maxHeight: number) {
    this.getScreenShot(quality, maxWidth, maxHeight).then(result => callback(undefined, result))
      .catch((err: BusinessError) => callback(err, undefined));
  }

  requestLayout(): void {
    this.uiOwner.requestLayout();
  }
}


