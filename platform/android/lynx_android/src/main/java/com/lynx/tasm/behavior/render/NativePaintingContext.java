// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import android.graphics.PointF;
import android.view.MotionEvent;
import androidx.annotation.NonNull;
import com.lynx.tasm.behavior.BehaviorRegistry;
import com.lynx.tasm.behavior.IPaintingContext;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.shadow.text.TextMeasurer;
import com.lynx.tasm.behavior.ui.ILynxUIMeaningfulContent;
import com.lynx.tasm.behavior.ui.MeaningfulPaintingArea;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.image.LynxImageManager;
import com.lynx.tasm.service.ILynxTextService.Page;
import java.util.ArrayList;
import java.util.List;

/**
 * Wrap the native object only to manage the lifetime on Java side.
 * All operations are implemented on the native object and called directly
 * by the pipeline.
 */
public class NativePaintingContext implements IPaintingContext {
  private static final int MEANINGFUL_PAINTING_RECORD_SIZE = 6;
  private static final int RECORD_INDEX_SIGN = 0;
  private static final int RECORD_INDEX_TYPE = 1;
  private static final int RECORD_INDEX_X = 2;
  private static final int RECORD_INDEX_Y = 3;
  private static final int RECORD_INDEX_WIDTH = 4;
  private static final int RECORD_INDEX_HEIGHT = 5;

  private long mNativePtr = 0;

  @NonNull private final PlatformRendererContext mPlatformRendererContext;
  private boolean mDestroyed = false;
  private long mTextra = 0;
  private LynxContext mContext;

  public NativePaintingContext(
      UIBody.UIBodyView rootView, LynxContext context, BehaviorRegistry behaviorRegistry) {
    mPlatformRendererContext = new PlatformRendererContext(rootView, context, behaviorRegistry);
    mContext = context;
    if (context.isTextServiceModeOn() && context.getTextService() != null) {
      mTextra = context.getTextService().createTextLayoutAPI(context);
    }
    mNativePtr = nativeCreatePaintingContext(this, mPlatformRendererContext.getNativePtr(),
        mPlatformRendererContext.getTextLayout(), mTextra);
  }

  @Override
  public void destroy() {
    if (mDestroyed) {
      return;
    }
    mDestroyed = true;

    if (mNativePtr != 0) {
      nativeDestroy(mNativePtr);
      mNativePtr = 0;
    }
    mPlatformRendererContext.destroy();
    // TextLayoutTextra owns mTextra and releases it on native teardown.
    mTextra = 0;
    mContext = null;
  }

  @Override
  public long getNativePaintingContextPtr() {
    return mNativePtr;
  }

  @Override
  public PointF convertPointInViewToScreen(int sign, PointF point) {
    return mPlatformRendererContext.convertPointInViewToScreen(sign, point);
  }

  public int getTargetWidth(int sign) {
    return mPlatformRendererContext.getTargetWidth(sign);
  }

  public int getTargetHeight(int sign) {
    return mPlatformRendererContext.getTargetHeight(sign);
  }

  public void attachUIBodyView(UIBody.UIBodyView view) {
    mPlatformRendererContext.setRootView(view);
  }

  @Override
  public void setLynxEngineActorForPlatformContextRef(long ptr) {
    if (mNativePtr == 0 || mDestroyed) {
      return;
    }
    nativeSetLynxEngineActorForPlatformContextRef(mNativePtr, ptr);
  }

  @Override
  public boolean dispatchPlatformMotionEvent(MotionEvent ev) {
    if (mNativePtr == 0 || mDestroyed) {
      return false;
    }

    int pointerCount = ev.getPointerCount();
    // iEventData: [event_type, action_type, event_source, pointer_count, ...]
    int[] iEventData = {0, ev.getActionMasked(), ev.getSource(), pointerCount};
    // fEventData: [pointer_id, pointer_x, pointer_y, ...]
    float[] fEventData = new float[pointerCount * 3];
    for (int i = 0; i < pointerCount; i++) {
      int base = i * 3;
      fEventData[base] = ev.getPointerId(i);
      fEventData[base + 1] = ev.getX(i);
      fEventData[base + 2] = ev.getY(i);
    }
    return nativeDispatchPlatformInputEvent(mNativePtr, iEventData, fEventData);
  }

  @Override
  public int getPlatformEventHandlerState() {
    if (mDestroyed || mNativePtr == 0) {
      return 0;
    }
    return nativeGetPlatformEventHandlerState(mNativePtr);
  }

  public List<MeaningfulPaintingArea> getMeaningfulPaintingAreas() {
    if (mDestroyed || mNativePtr == 0) {
      return new ArrayList<>();
    }

    // Native code only exports compact geometry records. Android resolves
    // validity and presented state here so the collector stays reusable for
    // other platforms.
    int[] records = nativeGetMeaningfulPaintingAreaRecords(mNativePtr);
    if (records == null || records.length == 0) {
      return new ArrayList<>();
    }

    ArrayList<MeaningfulPaintingArea> areas =
        new ArrayList<>(records.length / MEANINGFUL_PAINTING_RECORD_SIZE);
    for (int i = 0; i + MEANINGFUL_PAINTING_RECORD_SIZE <= records.length;
         i += MEANINGFUL_PAINTING_RECORD_SIZE) {
      int sign = records[i + RECORD_INDEX_SIGN];
      int type = records[i + RECORD_INDEX_TYPE];
      // Native collector returns geometry only. Java resolves whether the
      // content is already presented from the current Android render state.
      MeaningfulPaintingArea area =
          MeaningfulPaintingArea.create(records[i + RECORD_INDEX_X], records[i + RECORD_INDEX_Y],
              records[i + RECORD_INDEX_WIDTH], records[i + RECORD_INDEX_HEIGHT],
              isMeaningfulContentValid(type, sign), getMeaningfulContentStatus(type, sign), -1,
              mPlatformRendererContext.getMeaningfulPaintingAreaVisibleStatus(sign),
              mPlatformRendererContext.getMeaningfulPaintingAreaAlpha(sign),
              mPlatformRendererContext.getMeaningfulPaintingAreaScaleX(sign),
              mPlatformRendererContext.getMeaningfulPaintingAreaScaleY(sign));
      if (area != null) {
        areas.add(area);
      }
    }
    return areas;
  }

  private boolean isMeaningfulContentValid(int type, int sign) {
    if (type != PlatformRendererContext.PlatformRendererType.kImage) {
      return true;
    }
    LynxImageManager imageManager = mPlatformRendererContext.getImage(sign);
    return imageManager != null && imageManager.getHasContent();
  }

  private ILynxUIMeaningfulContent.MeaningfulContentStatus getMeaningfulContentStatus(
      int type, int sign) {
    if (type == PlatformRendererContext.PlatformRendererType.kImage) {
      LynxImageManager imageManager = mPlatformRendererContext.getImage(sign);
      if (imageManager != null && imageManager.getHasContent()) {
        return ILynxUIMeaningfulContent.MeaningfulContentStatus.PRESENTED;
      }
      return ILynxUIMeaningfulContent.MeaningfulContentStatus.PENDING;
    }

    if (type == PlatformRendererContext.PlatformRendererType.kText) {
      if (isTextPresented(sign)) {
        return ILynxUIMeaningfulContent.MeaningfulContentStatus.PRESENTED;
      }
      return ILynxUIMeaningfulContent.MeaningfulContentStatus.PENDING;
    }

    return ILynxUIMeaningfulContent.MeaningfulContentStatus.IRRELEVANT;
  }

  private boolean isTextPresented(int sign) {
    if (mContext != null && mContext.isTextServiceModeOn()) {
      // Text service publishes bundles after the platform text content has
      // been materialized, so bundle presence is enough to mark it presented.
      Page page = mPlatformRendererContext.getTextBundle(sign);
      return page != null;
    }

    // Non-text-service mode keeps layouts in TextMeasurer. A resolved layout
    // means the text content is already available for painting.
    TextMeasurer textMeasurer = mPlatformRendererContext.getTextMeasurer();
    return textMeasurer != null && textMeasurer.takeTextLayout(sign) != null;
  }

  private native long nativeCreatePaintingContext(
      NativePaintingContext jThis, long platformRendererContextPtr, Object textLayout, long textra);

  native void nativeSetLynxEngineActorForPlatformContextRef(long nativePtr, long ptr);

  native boolean nativeDispatchPlatformInputEvent(
      long nativePtr, int[] iEventData, float[] fEventData);

  native int nativeGetPlatformEventHandlerState(long nativePtr);

  native int[] nativeGetMeaningfulPaintingAreaRecords(long nativePtr);
  native void nativeDestroy(long nativePtr);
}
