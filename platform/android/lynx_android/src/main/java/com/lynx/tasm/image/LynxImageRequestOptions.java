// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.image;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringDef;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/** Request-specific options returned by {@link LynxImageRequestDelegate}. */
public final class LynxImageRequestOptions {
  public static final String PRIORITY_LOW = "low";
  public static final String PRIORITY_MEDIUM = "medium";
  public static final String PRIORITY_HIGH = "high";

  @StringDef({PRIORITY_LOW, PRIORITY_MEDIUM, PRIORITY_HIGH})
  @Retention(RetentionPolicy.SOURCE)
  public @interface RequestPriority {}

  private final @Nullable String mFinalUrl;
  private final @Nullable List<String> mFallbackUrls;
  private final boolean mUseRGB565;
  private final @Nullable @RequestPriority String mPriority;

  private LynxImageRequestOptions(@NonNull Builder builder) {
    mFinalUrl = builder.mFinalUrl;
    mFallbackUrls = builder.mFallbackUrls == null
        ? null
        : Collections.unmodifiableList(new ArrayList<>(builder.mFallbackUrls));
    mUseRGB565 = builder.mUseRGB565;
    mPriority = builder.mPriority;
  }

  @NonNull
  public static Builder newBuilder() {
    return new Builder();
  }

  @Nullable
  public String getFinalUrl() {
    return mFinalUrl;
  }

  @Nullable
  public List<String> getFallbackUrls() {
    return mFallbackUrls;
  }

  public boolean getUseRGB565() {
    return mUseRGB565;
  }

  @Nullable
  @RequestPriority
  public String getPriority() {
    return mPriority;
  }

  /** Builder for {@link LynxImageRequestOptions}. */
  public static final class Builder {
    private @Nullable String mFinalUrl;
    private @Nullable List<String> mFallbackUrls;
    private boolean mUseRGB565;
    private @Nullable @RequestPriority String mPriority;

    private Builder() {}

    @NonNull
    public Builder setFinalUrl(@NonNull String finalUrl) {
      mFinalUrl = finalUrl;
      return this;
    }

    @NonNull
    public Builder setFallbackUrls(@NonNull List<String> fallbackUrls) {
      mFallbackUrls = fallbackUrls;
      return this;
    }

    @NonNull
    public Builder setUseRGB565(boolean useRGB565) {
      mUseRGB565 = useRGB565;
      return this;
    }

    @NonNull
    public Builder setPriority(@NonNull @RequestPriority String priority) {
      mPriority = priority;
      return this;
    }

    @NonNull
    public LynxImageRequestOptions build() {
      return new LynxImageRequestOptions(this);
    }
  }
}
