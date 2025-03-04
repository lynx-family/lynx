// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

/**
 * Class to control the behavior when constructing TemplateBundle from template
 */
public final class TemplateBundleOption {
  private int mContextPoolSize = 0;
  private boolean mEnableContextAutoRefill = false;

  private TemplateBundleOption(int contextPoolSize, boolean enableContextAutoRefill) {
    mContextPoolSize = contextPoolSize;
    mEnableContextAutoRefill = enableContextAutoRefill;
  }

  public int getContextPoolSize() {
    return mContextPoolSize;
  }

  public boolean getEnableContextAutoRefill() {
    return mEnableContextAutoRefill;
  }

  public static class Builder {
    private int mContextPoolSize = 0;
    private boolean mEnableContextAutoRefill = false;

    /**
     * Pre-create a certain number of lepus contexts, which could speed up loadTemplateBundle.
     * Notice:
     * 1. one context will be pre-created in TemplateBundle without actively calling if FE marks
     * `enableUseContextPool` true, in which case there is no need to actively call this method.
     * 2. not all TemplateBundles are able to pre-create context, it is only applicable to templates
     * using lepusNG.
     * 3. this API will override the FE settings
     * Default value: 0
     */
    public Builder setContextPoolSize(int size) {
      this.mContextPoolSize = size;
      return this;
    }

    /**
     * Whether to automatically replenish the context after it is consumed
     * Default value: false
     */
    public Builder setEnableContextAutoRefill(boolean enable) {
      this.mEnableContextAutoRefill = enable;
      return this;
    }

    public TemplateBundleOption build() {
      return new TemplateBundleOption(mContextPoolSize, mEnableContextAutoRefill);
    }
  }
}
