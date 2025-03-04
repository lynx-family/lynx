// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui;

import android.graphics.Color;
import androidx.annotation.RestrictTo;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.behavior.StyleConstants;
import java.util.ArrayList;
import java.util.List;

@RestrictTo(RestrictTo.Scope.LIBRARY)
public class ShadowData {
  public int color;
  public float offsetX;
  public float offsetY;
  public float blurRadius;
  public float spreadRadius;
  // TODO(linxs): remove this.
  public float blurRenderRadiusExtent;
  // FIXME(linxs): support all this.
  public int option = StyleConstants.SHADOW_OPTION_NONE;

  @Override
  public String toString() {
    return "ShadowData: "
        + "Color: red " + Color.red(color) + " green: " + Color.green(color) + " blue: "
        + Color.blue(Color.blue(color)) + " OffsetX: " + offsetX + " offsetY: " + offsetY
        + " blurRadius: " + blurRadius + " spreadRadius: " + spreadRadius + "option: " + option;
  };

  public ShadowData() {}

  public ShadowData(ShadowData shadow) {
    this.color = shadow.color;
    this.offsetX = shadow.offsetX;
    this.offsetY = shadow.offsetY;
    this.blurRadius = shadow.blurRadius;
    this.spreadRadius = shadow.spreadRadius;
    this.blurRenderRadiusExtent = shadow.blurRenderRadiusExtent;
    this.option = shadow.option;
  }

  public boolean isInset() {
    return option == StyleConstants.SHADOW_OPTION_INSET;
  }

  @Override
  public boolean equals(Object o) {
    if (this == o) {
      return true;
    }
    if (o == null || getClass() != o.getClass()) {
      return false;
    }

    ShadowData boxShadow = (ShadowData) o;

    if (color != boxShadow.color) {
      return false;
    }
    if (Float.compare(boxShadow.offsetX, offsetX) != 0) {
      return false;
    }
    if (Float.compare(boxShadow.offsetY, offsetY) != 0) {
      return false;
    }
    if (Float.compare(boxShadow.blurRadius, blurRadius) != 0) {
      return false;
    }
    if (Float.compare(boxShadow.spreadRadius, spreadRadius) != 0) {
      return false;
    }
    if (Float.compare(boxShadow.blurRenderRadiusExtent, blurRenderRadiusExtent) != 0) {
      return false;
    }
    return option == boxShadow.option;
  }

  @Override
  public int hashCode() {
    int result = option;
    result = 31 * result + color;
    result = 31 * result + (offsetX != +0.0f ? Float.floatToIntBits(offsetX) : 0);
    result = 31 * result + (offsetY != +0.0f ? Float.floatToIntBits(offsetY) : 0);
    result = 31 * result + (blurRadius != +0.0f ? Float.floatToIntBits(blurRadius) : 0);
    result = 31 * result + (spreadRadius != +0.0f ? Float.floatToIntBits(spreadRadius) : 0);
    result = 31 * result
        + (blurRenderRadiusExtent != +0.0f ? Float.floatToIntBits(blurRenderRadiusExtent) : 0);
    return result;
  }

  public static List<ShadowData> parseShadow(ReadableArray shadows) {
    if (shadows == null || shadows.size() == 0) {
      return null;
    }
    ArrayList<ShadowData> shadowList = new ArrayList<>();
    for (int i = 0; i < shadows.size(); i++) {
      ReadableArray shadow = shadows.getArray(i);
      ShadowData boxShadow = new ShadowData();
      boxShadow.offsetX = (float) shadow.getDouble(0);
      boxShadow.offsetY = (float) shadow.getDouble(1);
      boxShadow.blurRadius = (float) shadow.getDouble(2);
      boxShadow.blurRenderRadiusExtent = boxShadow.blurRadius * 1.25f;
      boxShadow.spreadRadius = (float) shadow.getDouble(3);
      boxShadow.option = shadow.getInt(4);
      boxShadow.color = (int) shadow.getLong(5);
      if (checkIsValidShadowData(boxShadow)) {
        shadowList.add(boxShadow);
      }
    }
    return shadowList;
  }

  private static boolean checkIsValidShadowData(ShadowData shadow) {
    // Case 1: offsetX,offsetY,blur,spread all are zero
    // Case 2: color is transparent
    // Case 3: blurRadius is negative
    if ((shadow.offsetX == .0f && shadow.offsetY == .0f && shadow.blurRadius == .0f
            && shadow.spreadRadius == .0f)
        || (Color.alpha(shadow.color) == 0) || (shadow.blurRadius < .0f)) {
      return false;
    }
    return true;
  }
}
