// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.service;

import android.content.Context;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.behavior.ui.background.BackgroundLayerDrawable;

/**
 * Lynx extension for {@link ILynxImageService}. Optional implementation.
 */
@Keep
public interface ILynxImageServiceExtension extends IServiceExtension {
  /**
   * The implementation of background-image: url
   * now provides a default {@link com.lynx.tasm.behavior.ui.image.BackgroundImageDrawable}
   * implementation.
   *
   * @param context The Android context.
   * @param url The URL of the background image.
   * @return A BackgroundLayerDrawable representing the background image.
   */
  @Nullable
  BackgroundLayerDrawable createBackgroundImageDrawable(
      @NonNull Context context, @NonNull String url);

  /**
   * notify onLynxEnvSetup
   */
  void onLynxEnvSetup();
}
