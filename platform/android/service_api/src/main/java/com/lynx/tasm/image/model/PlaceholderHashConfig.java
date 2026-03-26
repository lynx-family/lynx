// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.image.model;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.util.Objects;

public class PlaceholderHashConfig {
  public final boolean isPreView;
  public final @NonNull String hash;
  public final @Nullable String metaData;
  public final int width;
  public final int height;
  public final int radius;
  public final int iterations;

  public PlaceholderHashConfig(boolean isPreView, @NonNull String hash, @Nullable String metaData,
      int width, int height, int radius, int iterations) {
    this.isPreView = isPreView;
    this.hash = hash;
    this.metaData = metaData;
    this.width = width;
    this.height = height;
    this.radius = radius;
    this.iterations = iterations;
  }

  @Override
  public boolean equals(Object o) {
    if (this == o) {
      return true;
    }
    if (o == null || getClass() != o.getClass()) {
      return false;
    }
    PlaceholderHashConfig that = (PlaceholderHashConfig) o;
    return isPreView == that.isPreView && width == that.width && height == that.height
        && radius == that.radius && iterations == that.iterations && hash.equals(that.hash);
  }

  @Override
  public int hashCode() {
    return Objects.hash(isPreView, hash, width, height, radius, iterations);
  }
}
