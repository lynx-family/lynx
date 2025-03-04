// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import java.util.Set;
import org.json.JSONObject;

/**
 * this class will be removed
 */
public class LynxConfigInfo {
  private final String mPageVersion;
  private final String mPageType;
  private final String mCliVersion;
  private final String mCustomData; // json string
  private final String mTemplateUrl;
  private final String mTargetSdkVersion;
  private final String mLepusVersion;
  private final ThreadStrategyForRendering mThreadStrategyForRendering;
  private final boolean mEnableLepusNG;
  private final String mRadonMode;
  private final String mReactVersion;
  private final Set<String> mRegisteredComponent;
  private final boolean mCssAlignWithLegacyW3c;
  private final boolean mEnableCSSParser;

  private LynxConfigInfo(Builder builder) {
    this.mPageVersion = builder.mPageVersion;
    this.mPageType = builder.mPageType;
    this.mCliVersion = builder.mCliVersion;
    this.mCustomData = builder.mCustomData;
    this.mTemplateUrl = builder.mTemplateUrl;
    this.mTargetSdkVersion = builder.mTargetSdkVersion;
    this.mLepusVersion = builder.mLepusVersion;
    this.mThreadStrategyForRendering = builder.mThreadStrategyForRendering;
    this.mEnableLepusNG = builder.mEnableLepusNG;
    this.mRadonMode = builder.mRadonMode;
    this.mReactVersion = builder.mReactVersion;
    this.mRegisteredComponent = builder.mRegisteredComponent;
    this.mCssAlignWithLegacyW3c = builder.mCssAlignWithLegacyW3c;
    this.mEnableCSSParser = builder.mEnableCSSParser;
  }

  public String getPageVersion() {
    return mPageVersion;
  }

  public String getPageType() {
    return mPageType;
  }

  public String getCliVersion() {
    return mCliVersion;
  }

  public String getCustomData() {
    return mCustomData;
  }

  public String getTemplateUrl() {
    return mTemplateUrl;
  }

  public String getTargetSdkVersion() {
    return mTargetSdkVersion;
  }

  public String getLepusVersion() {
    return mLepusVersion;
  }

  public ThreadStrategyForRendering getThreadStrategyForRendering() {
    return mThreadStrategyForRendering;
  }

  public boolean isEnableLepusNG() {
    return mEnableLepusNG;
  }

  public String getRadonMode() {
    return mRadonMode;
  }

  public String getReactVersion() {
    return mReactVersion;
  }

  // all registered component
  public Set<String> getRegisteredComponent() {
    return mRegisteredComponent;
  }

  public boolean getCssAlignWithLegacyW3c() {
    return mCssAlignWithLegacyW3c;
  }

  // get LynxConfigInfo key value in json dictionary
  public JSONObject toJson() {
    JSONObject ret = new JSONObject();
    try {
      ret.put("pageVersion", mPageVersion);
      ret.put("pageType", mPageType);
      ret.put("cliVersion", mCliVersion);
      ret.put("customData", mCustomData);
      ret.put("templateUrl", mTemplateUrl);
      ret.put("targetSdkVersion", mTargetSdkVersion);
      ret.put("lepusVersion", mLepusVersion);
      ret.put("isEnableLepusNG", mEnableLepusNG);
      ret.put("radonMode", mRadonMode);
      ret.put("reactVersion", mReactVersion);
      ret.put("threadStrategyForRendering",
          mThreadStrategyForRendering != null ? mThreadStrategyForRendering.id() : 0);
      ret.put("registeredComponent", mRegisteredComponent);
      ret.put("cssAlignWithLegacyW3c", mCssAlignWithLegacyW3c);
      ret.put("cssParser", mEnableCSSParser);
    } catch (Throwable e) {
      e.printStackTrace();
    }
    return ret;
  }

  public static class Builder {
    private String mPageVersion;
    private String mPageType;
    private String mCliVersion;
    private String mCustomData; // json string
    private String mTemplateUrl;
    private String mTargetSdkVersion;
    private String mLepusVersion;
    private ThreadStrategyForRendering mThreadStrategyForRendering;
    private boolean mEnableLepusNG;
    private String mRadonMode;
    private String mReactVersion;
    private Set<String> mRegisteredComponent;
    private boolean mCssAlignWithLegacyW3c;
    private boolean mEnableCSSParser;

    public Builder() {
      mPageVersion = "error";
      mPageType = "error";
      mCliVersion = "error";
      mCustomData = "error";
      mTemplateUrl = "error";
      mTargetSdkVersion = "error";
      mLepusVersion = "error";
      mThreadStrategyForRendering = null;
      mEnableLepusNG = false;
      mRadonMode = "error";
      mReactVersion = "error";
      mRegisteredComponent = null;
      mCssAlignWithLegacyW3c = false;
      mEnableCSSParser = false;
    }

    public LynxConfigInfo buildLynxConfigInfo() {
      return new LynxConfigInfo(this);
    }

    public Builder setPageVersion(String pageVersion) {
      mPageVersion = pageVersion;
      return this;
    }

    public Builder setPageType(String pageType) {
      mPageType = pageType;
      return this;
    }

    public Builder setCliVersion(String cliVersion) {
      mCliVersion = cliVersion;
      return this;
    }

    public Builder setCustomData(String customData) {
      mCustomData = customData;
      return this;
    }

    public Builder setTemplateUrl(String templateUrl) {
      mTemplateUrl = templateUrl;
      return this;
    }

    public Builder setTargetSdkVersion(String targetSdkVersion) {
      mTargetSdkVersion = targetSdkVersion;
      return this;
    }

    public Builder setLepusVersion(String lepusVersion) {
      mLepusVersion = lepusVersion;
      return this;
    }

    public Builder setThreadStrategyForRendering(
        ThreadStrategyForRendering threadStrategyForRendering) {
      mThreadStrategyForRendering = threadStrategyForRendering;
      return this;
    }

    public Builder setEnableLepusNG(boolean enableLepusNG) {
      mEnableLepusNG = enableLepusNG;
      return this;
    }

    public Builder setRadonMode(String radonMode) {
      mRadonMode = radonMode;
      return this;
    }

    public Builder setReactVersion(String reactVersion) {
      mReactVersion = reactVersion;
      return this;
    }

    public Builder setRegisteredComponent(Set<String> registeredComponent) {
      mRegisteredComponent = registeredComponent;
      return this;
    }

    public Builder setCssAlignWithLegacyW3c(boolean cssAlignWithLegacyW3c) {
      mCssAlignWithLegacyW3c = cssAlignWithLegacyW3c;
      return this;
    }

    public Builder setEnableCSSParser(boolean enable) {
      mEnableCSSParser = enable;
      return this;
    }
  }
}
