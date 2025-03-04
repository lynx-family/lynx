// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import java.util.EnumSet;
import java.util.Map;

// if both templateBundle and binaryData exist, use templateBundle first
// LynxLoadMode represents current loadTemplate's main pipeline mode, which
// contains different lifecycle control
// LynxLoadOption represents switches for Lynx's specific atomic abilities
public final class LynxLoadMeta {
  String url;
  byte[] binaryData;
  TemplateBundle bundle;
  TemplateData initialData;
  TemplateData globalProps;
  LynxLoadMode loadMode;
  EnumSet<LynxLoadOption> loadOptions;
  Map<String, String> lynxViewConfig;

  private LynxLoadMeta(String url, byte[] binaryData, TemplateBundle bundle,
      TemplateData initialData, TemplateData globalProps, LynxLoadMode loadMode,
      EnumSet<LynxLoadOption> loadOptions, Map<String, String> lynxViewConfig) {
    this.url = url;
    this.binaryData = binaryData;
    this.bundle = bundle;
    this.initialData = initialData;
    this.globalProps = globalProps;
    this.loadMode = loadMode;
    this.loadOptions = loadOptions;
    this.lynxViewConfig = lynxViewConfig;
  }

  public String getUrl() {
    return this.url;
  }

  /**
   * Check whether the incoming bundle is valid.
   */
  public boolean isBundleValid() {
    return this.bundle != null && this.bundle.isValid();
  }

  /**
   * Check whether the incoming binary is valid.
   */
  public boolean isBinaryValid() {
    return this.binaryData != null && this.binaryData.length > 0;
  }

  public byte[] getTemplateBinary() {
    return this.binaryData;
  }

  public TemplateBundle getTemplateBundle() {
    return this.bundle;
  }

  public TemplateData getInitialData() {
    return this.initialData;
  }

  public TemplateData getGlobalProps() {
    return globalProps;
  }

  public Map<String, String> getLynxViewConfig() {
    return lynxViewConfig;
  }

  public LynxLoadMode getLoadMode() {
    return this.loadMode;
  }

  public EnumSet<LynxLoadOption> getLoadOption() {
    return this.loadOptions;
  }

  public boolean enableDumpElementTree() {
    return this.loadOptions.contains(LynxLoadOption.DUMP_ELEMENT);
  }

  public boolean enableRecycleTemplateBundle() {
    return this.loadOptions.contains(LynxLoadOption.RECYCLE_TEMPLATE_BUNDLE);
  }

  public boolean enableProcessLayout() {
    return this.loadOptions.contains(LynxLoadOption.PROCESS_LAYOUT_WITHOUT_UI_FLUSH);
  }

  public static class Builder {
    private String url;
    private byte[] binaryData;
    private TemplateBundle bundle;
    private TemplateData initialData;
    private TemplateData globalProps;
    private LynxLoadMode loadMode;
    private EnumSet<LynxLoadOption> loadOptions = EnumSet.noneOf(LynxLoadOption.class);

    private Map<String, String> lynxViewConfig;

    public void setUrl(String url) {
      this.url = url;
    }

    public void setBinaryData(byte[] binaryData) {
      this.binaryData = binaryData;
    }

    public void setTemplateBundle(TemplateBundle bundle) {
      this.bundle = bundle;
    }

    public void setInitialData(TemplateData initialData) {
      this.initialData = initialData;
    }
    public void setGlobalProps(TemplateData globalProps) {
      this.globalProps = globalProps;
    }

    public void setLoadMode(LynxLoadMode loadMode) {
      this.loadMode = loadMode;
    }

    public void addLoadOption(LynxLoadOption loadOption) {
      this.loadOptions.add(loadOption);
    }

    public void setLynxViewConfig(Map<String, String> lynxViewConfig) {
      this.lynxViewConfig = lynxViewConfig;
    }

    public LynxLoadMeta build() {
      return new LynxLoadMeta(this.url, this.binaryData, this.bundle, this.initialData,
          this.globalProps, this.loadMode, this.loadOptions, this.lynxViewConfig);
    }
  }
}
