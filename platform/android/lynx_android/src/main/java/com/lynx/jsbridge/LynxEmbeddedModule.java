// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge;

import android.content.Context;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.LynxUpdateMeta;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.TemplateData;
import com.lynx.tasm.group.ILynxViewGroup;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;

public class LynxEmbeddedModule extends LynxModule {
  public static final String NAME = "LynxEmbeddedModule";

  private WeakReference<ILynxViewGroup> mLynxViewGroupRef;

  public LynxEmbeddedModule(Context context, Object param) {
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
  public void updateData(final int lynxViewId, final ReadableMap params, final Callback resolve) {
    LynxView lynxView = getLynxViewById(lynxViewId);
    if (lynxView == null) {
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
  public TemplateData getData(final int lynxViewId) {
    LynxView lynxView = getLynxViewById(lynxViewId);
    if (lynxView != null) {
      return lynxView.getTemplateData();
    }
    return null;
  }
}
