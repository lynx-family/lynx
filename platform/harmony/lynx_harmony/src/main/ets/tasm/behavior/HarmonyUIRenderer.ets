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
const COMPONENT_SNAPSHOT_MAX_SIZE = 8191;
const COMPONENT_SNAPSHOT_TILE_SIZE = 2048;
const BYTES_PER_32_BIT_PIXEL = 4;

function getScaleToMaxWidth(imageWidth: number, maxWidth: number): number {
  if (imageWidth <= 0) {
    return 1;
  }

  let scale = 1;
  if (maxWidth > 0 && imageWidth > maxWidth) {
    scale = Math.min(scale, maxWidth / imageWidth);
  }
  return Math.max(0, Math.min(scale, 1));
}

function getComponentSnapshotScale(imageWidth: number, imageHeight: number): number {
  if (imageWidth <= 0 || imageHeight <= 0) {
    return 1;
  }

  let scale = 1;
  if (imageWidth > COMPONENT_SNAPSHOT_MAX_SIZE || imageHeight > COMPONENT_SNAPSHOT_MAX_SIZE) {
    scale = Math.min(scale, COMPONENT_SNAPSHOT_MAX_SIZE / imageWidth,
      COMPONENT_SNAPSHOT_MAX_SIZE / imageHeight);
  }
  return Math.max(0, Math.min(scale, 1));
}

function shouldUseTiledSnapshot(imageWidth: number, imageHeight: number): boolean {
  return imageWidth > COMPONENT_SNAPSHOT_MAX_SIZE || imageHeight > COMPONENT_SNAPSHOT_MAX_SIZE;
}

function isSupportedTiledPixelFormat(pixelFormat: image.PixelMapFormat): boolean {
  return pixelFormat === image.PixelMapFormat.RGBA_8888 ||
    pixelFormat === image.PixelMapFormat.BGRA_8888;
}

function stringifyError(error: Object): string {
  const errorMessage = JSON.stringify(error);
  return errorMessage === "{}" ? error.toString() : errorMessage;
}

async function getComponentSnapshotPixelMap(componentId: string, uiContext: UIContext | undefined,
  options: componentSnapshot.SnapshotOptions): Promise<image.PixelMap> {
  if (uiContext !== undefined) {
    return await uiContext.getComponentSnapshot().get(componentId, options);
  }
  return await componentSnapshot.get(componentId, options);
}

async function getTiledComponentSnapshot(componentId: string, uiContext: UIContext | undefined,
  imageWidth: number, imageHeight: number): Promise<image.PixelMap> {
  const snapshotWidth = Math.ceil(imageWidth);
  const snapshotHeight = Math.ceil(imageHeight);
  let resultPixelMap: image.PixelMap | undefined;
  try {
    for (let top = 0; top < snapshotHeight; top += COMPONENT_SNAPSHOT_TILE_SIZE) {
      const bottom = Math.min(top + COMPONENT_SNAPSHOT_TILE_SIZE, snapshotHeight);
      for (let left = 0; left < snapshotWidth; left += COMPONENT_SNAPSHOT_TILE_SIZE) {
        const right = Math.min(left + COMPONENT_SNAPSHOT_TILE_SIZE, snapshotWidth);
        let tilePixelMap: image.PixelMap | undefined;
        try {
          tilePixelMap = await getComponentSnapshotPixelMap(componentId, uiContext, {
            scale: 1,
            region: {
              left: left,
              right: right,
              top: top,
              bottom: bottom
            }
          });
          const tileInfo = tilePixelMap.getImageInfoSync();
          if (!isSupportedTiledPixelFormat(tileInfo.pixelFormat)) {
            throw new Error("Unsupported tiled snapshot pixel format " + tileInfo.pixelFormat);
          }
          const tileWidth = Math.floor(Math.min(tileInfo.size.width, right - left));
          const tileHeight = Math.floor(Math.min(tileInfo.size.height, bottom - top));
          if (tileWidth <= 0 || tileHeight <= 0) {
            continue;
          }
          if (resultPixelMap === undefined) {
            resultPixelMap = image.createPixelMapSync({
              size: { width: snapshotWidth, height: snapshotHeight },
              pixelFormat: tileInfo.pixelFormat,
              editable: true
            });
          }
          const pixels = new ArrayBuffer(tileWidth * tileHeight * BYTES_PER_32_BIT_PIXEL);
          const tileArea: image.PositionArea = {
            pixels: pixels,
            offset: 0,
            stride: tileWidth * BYTES_PER_32_BIT_PIXEL,
            region: { x: 0, y: 0, size: { width: tileWidth, height: tileHeight } }
          };
          tilePixelMap.readPixelsSync(tileArea);
          const resultArea: image.PositionArea = {
            pixels: pixels,
            offset: 0,
            stride: tileWidth * BYTES_PER_32_BIT_PIXEL,
            region: { x: left, y: top, size: { width: tileWidth, height: tileHeight } }
          };
          resultPixelMap.writePixelsSync(resultArea);
        } finally {
          tilePixelMap?.release();
        }
      }
    }
    if (resultPixelMap === undefined) {
      throw new Error("Tiled snapshot result is empty");
    }
    return resultPixelMap;
  } catch (e) {
    resultPixelMap?.release();
    throw new Error(stringifyError(e));
  }
}

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
      const componentId = this.uiOwner.getId();
      let imageWidth = 0;
      let imageHeight = 0;
      const uiContext = this.uiOwner.uiContext;
      try {
        const componentInfo = uiContext?.getComponentUtils().getRectangleById(componentId);
        imageWidth = Math.ceil(componentInfo?.size.width ?? 0);
        imageHeight = Math.ceil(componentInfo?.size.height ?? 0);
      } catch (e) {
        LLog.d(TAG, "Devtool takeScreenShot get component info fail " + stringifyError(e));
      }
      if (shouldUseTiledSnapshot(imageWidth, imageHeight)) {
        try {
          pixelMap = await getTiledComponentSnapshot(componentId, uiContext, imageWidth, imageHeight);
        } catch (e) {
          LLog.d(TAG, "Devtool takeScreenShot tiled snapshot fail " + stringifyError(e));
        }
      }
      if (pixelMap === undefined) {
        const snapshotScale = getComponentSnapshotScale(imageWidth, imageHeight);
        pixelMap = await getComponentSnapshotPixelMap(componentId, uiContext, { scale: snapshotScale });
      }
      imagePackerApi = image.createImagePacker();
      const imageInfo = pixelMap.getImageInfoSync();
      const snapshotWidth = imageInfo.size.width;
      const snapshotHeight = imageInfo.size.height;
      if (imageWidth <= 0 || imageHeight <= 0) {
        imageWidth = snapshotWidth;
        imageHeight = snapshotHeight;
      }
      const postSnapshotScale = getScaleToMaxWidth(snapshotWidth, maxWidth);
      if (postSnapshotScale !== 1) {
        pixelMap.scaleSync(postSnapshotScale, postSnapshotScale);
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
      LLog.d(TAG, errorMsg + stringifyError(e));
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

  keyboardStatusChanged(height: number): void {
    this.uiOwner.keyboardStatusChanged(height);
  }

  setAvoidKeyboardListener(avoidKeyboardListener?: (translateY: number, isKeyboardTransition: boolean) => void): void {
    this.uiOwner.setAvoidKeyboardListener(avoidKeyboardListener);
  }

  onEnterForeground(): void {
    this.uiOwner.onEnterForeground();
  }

  onEnterBackground(): void {
    this.uiOwner.onEnterBackground();
  }
}

