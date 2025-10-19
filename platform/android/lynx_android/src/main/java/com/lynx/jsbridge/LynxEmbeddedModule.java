// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge;

import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.LynxUpdateMeta;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.TemplateData;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.group.ILynxViewGroup;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;

public class LynxEmbeddedModule extends LynxContextModule {
  public static final String NAME = "LynxEmbeddedModule";

  private WeakReference<ILynxViewGroup> mLynxViewGroupRef;

  public LynxEmbeddedModule(LynxContext context, Object param) {
    super(context);
    if (param instanceof ILynxViewGroup) {
      mLynxViewGroupRef = new WeakReference<>((ILynxViewGroup) param);
    }
  }

  private LynxView getLynxViewById(final int lynxViewId) {
    if (mLynxViewGroupRef != null) {
      final ILynxViewGroup group = mLynxViewGroupRef.get();
      if (group != null) {
        return group.getLynxViewById(lynxViewId);
      }
    }
    return null;
  }

  @LynxMethod
  public void updateData(final int lynxViewId, final ReadableMap params, final Callback resolve,
      final Callback reject) {
    LynxView lynxView = getLynxViewById(lynxViewId);
    if (lynxView == null) {
      JavaOnlyMap error = new JavaOnlyMap();
      error.put("message", "Cannot get related lynxView (ID: " + lynxViewId + ")");
      reject.invoke(error);
      return;
    }
    UIThreadUtils.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        TemplateData data = TemplateData.fromMap(params.asHashMap());
        LynxUpdateMeta meta = new LynxUpdateMeta.Builder().setUpdatedData(data).build();
        lynxView.updateMetaData(meta);
        resolve.invoke();
      }
    });
  }
  @LynxMethod
  public void getData(final int lynxViewId, final Callback resolve, final Callback reject) {
    LynxView lynxView = getLynxViewById(lynxViewId);
    if (lynxView == null) {
      JavaOnlyMap error = new JavaOnlyMap();
      error.put("message", "Cannot get related lynxView (ID: " + lynxViewId + ")");
      reject.invoke(error);
      return;
    }
    UIThreadUtils.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        TemplateData data = lynxView.getTemplateData();
        resolve.invoke(data);
      }
    });
  }

  @LynxMethod
  public void setDataV2(final ReadableMap params, final Callback resolve, final Callback reject) {
    LynxView lynxView = mLynxContext.getLynxView();
    if (lynxView == null) {
      JavaOnlyMap error = new JavaOnlyMap();
      error.put("message", "Cannot get related lynxView.");
      reject.invoke(error);
      return;
    }

    UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
      @Override
      public void run() {
        TemplateData data = TemplateData.fromMap(params.asHashMap());
        LynxUpdateMeta updateMeta = new LynxUpdateMeta.Builder().setUpdatedData(data).build();
        lynxView.updateMetaData(updateMeta);
        resolve.invoke();
      }
    });
  }

  @LynxMethod
  public void getDataV2(final Callback resolve, final Callback reject) {
    LynxView lynxView = mLynxContext.getLynxView();
    if (lynxView == null) {
      JavaOnlyMap error = new JavaOnlyMap();
      error.put("message", "Cannot get related lynxView.");
      reject.invoke(error);
      return;
    }

    UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
      @Override
      public void run() {
        TemplateData data = lynxView.getTemplateData();
        resolve.invoke(data);
      }
    });
  }
}
