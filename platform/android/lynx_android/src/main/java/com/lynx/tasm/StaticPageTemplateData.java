// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import androidx.annotation.NonNull;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.base.trace.TraceEventDef;
import java.util.Map;

final class StaticPageTemplateData extends TemplateData {
  private static final String TAG = "LynxTemplateData";

  @NonNull private final Map<String, Object> mPlatformData;

  StaticPageTemplateData(@NonNull Map<String, Object> data) {
    super();
    mPlatformData = data;
    markReadOnly();
  }

  @Override
  public Map<Object, Object> toMap() {
    @SuppressWarnings("unchecked")
    Map<Object, Object> result = (Map<Object, Object>) (Map<?, ?>) mPlatformData;
    return result;
  }

  @Override
  public boolean checkIsLegalData() {
    return true;
  }

  @Override
  public long getNativePtr() {
    return 0;
  }

  @Override
  public void flush() {}

  @Override
  public void updateWithTemplateData(TemplateData diff) {
    LLog.e(TAG, "updateWithTemplateData is not supported for static-page data");
  }

  @Override
  public void markState(String name) {
    LLog.e(TAG, "markState is not supported for static-page data");
  }

  @Override
  public TemplateData deepClone() {
    return new StaticPageTemplateData(mPlatformData);
  }

  @Override
  public TemplateData shallowClone() {
    TraceEvent.beginSection(TraceEventDef.TEMPLATE_DATA_SHALLOW_CLONE);
    TemplateData data = new StaticPageTemplateData(mPlatformData);
    TraceEvent.endSection(TraceEventDef.TEMPLATE_DATA_SHALLOW_CLONE);
    return data;
  }

  @Override
  public boolean isEmpty() {
    return mPlatformData.isEmpty();
  }
}
