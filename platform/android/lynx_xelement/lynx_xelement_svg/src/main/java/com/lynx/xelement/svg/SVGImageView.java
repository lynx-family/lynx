// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.svg;
import android.content.Context;
import android.graphics.drawable.Drawable;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.AppCompatImageView;
import androidx.core.view.ViewCompat;

public class SVGImageView extends AppCompatImageView {
  public SVGImageView(Context context) {
    super(context);
  }

  @Override
  public void setImageDrawable(@Nullable Drawable drawable) {
    setSoftwareLayerType();
    super.setImageDrawable(drawable);
  }

  //===============================================================================================

  private void setSoftwareLayerType() {
    ViewCompat.setLayerType(this, ViewCompat.LAYER_TYPE_SOFTWARE, null);
  }
}
