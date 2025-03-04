// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import androidx.annotation.Keep;
import com.lynx.BuildConfig;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.ReadableMapKeySetIterator;
import com.lynx.tasm.base.LLog;
import java.util.HashMap;
import org.json.JSONException;
import org.json.JSONObject;

// Deprecated, will be removed later
public final class LynxPerfMetric {
  public LynxPerfMetric(ReadableMap metric, ReadableMap timingMetric, String url, String pageType,
      String reactVersion) {}

  public JSONObject toJSONObject() {
    return new JSONObject();
  }

  public double getTasmEndDecodeFinishLoadTemplate() {
    return 0;
  }

  public double getCoreJSSize() {
    return 0;
  }

  public double getRenderPage() {
    return 0;
  }

  public double getTasmBinaryDecode() {
    return 0;
  }

  public double getDiffSameRoot() {
    return 0;
  }

  public double getJsAndTasmAllReady() {
    return 0;
  }

  public double getJsFinishLoadCore() {
    return 0;
  }

  public double getJsRuntimeType() {
    return 0;
  }

  public double getDiffRootCreate() {
    return 0;
  }

  public double getLayout() {
    return 0;
  }

  public double getTasmFinishLoadTemplate() {
    return 0;
  }

  public double getJsFinishLoadApp() {
    return 0;
  }

  public double getTti() {
    return 0;
  }

  public double getFirsPageLayout() {
    return 0;
  }

  public boolean isHasActualFMP() {
    return false;
  }

  public double getActualFMPDuration() {
    return 0;
  }

  public double getActualFirstScreenEndTimeStamp() {
    return 0;
  }

  public double getSsrFmp() {
    return 0;
  }
  public double getSsrDispatch() {
    return 0;
  }
  public double getSsrGenerateDom() {
    return 0;
  }
  public double getSsrSourceSize() {
    return 0;
  }
  public boolean getIsSsrHydrate() {
    return false;
  }

  public void setIsColdBoot(boolean is) {}

  public void correctFirstPageLayout(long timestamp) {}

  public void setInitTiming(long start, long end) {}

  @Override
  public String toString() {
    return "";
  }
}
