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
  private boolean mSkipCSS = false;
  private final String mUrl;
  private boolean mDebuggable = false;

  private TemplateBundleOption(int contextPoolSize, boolean enableContextAutoRefill,
      boolean skipCSS, String url, boolean debuggable) {
    mContextPoolSize = contextPoolSize;
    mEnableContextAutoRefill = enableContextAutoRefill;
    mSkipCSS = skipCSS;
    mUrl = url;
    mDebuggable = debuggable;
  }

  public int getContextPoolSize() {
    return mContextPoolSize;
  }

  public String getUrl() {
    return mUrl;
  }

  public boolean getEnableContextAutoRefill() {
    return mEnableContextAutoRefill;
  }

  public boolean getDebuggable() {
    return mDebuggable;
  }

  public boolean getSkipCSS() {
    return mSkipCSS;
  }

  public static class Builder {
    private String mUrl;
    private int mContextPoolSize = 0;
    private boolean mEnableContextAutoRefill = false;
    private boolean mDebuggable = false;
    private boolean mSkipCSS = false;

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

    /**
     * Whether to skip CSS decoding when constructing TemplateBundle.
     * Default value: false
     */
    public Builder setSkipCSS(boolean enable) {
      this.mSkipCSS = enable;
      return this;
    }

    /**
     * Set the source-url of the loading template.
     * @param url sourceUrl;
     * @return Builder;
     */
    public Builder setUrl(String url) {
      this.mUrl = url;
      return this;
    }

    /**
     * A TemplateBundle can be debugged in two cases:
     * 1. The switch `LynxEnv.inst().isDevtoolEnabled()` is `true`.
     * 2. The switch `LynxEnv.inst().isDevtoolEnabled()` is `false` and this value is `true`.
     * Default value: false
     */
    public Builder setDebuggable(boolean debuggable) {
      this.mDebuggable = debuggable;
      return this;
    }

    public TemplateBundleOption build() {
      return new TemplateBundleOption(
          mContextPoolSize, mEnableContextAutoRefill, mSkipCSS, mUrl, mDebuggable);
    }
  }
}
