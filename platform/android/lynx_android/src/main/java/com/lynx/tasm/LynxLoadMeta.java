// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import java.nio.ByteBuffer;
import java.util.EnumSet;
import java.util.Map;

// if both templateBundle and binaryData exist, use templateBundle first
// LynxLoadMode represents current loadTemplate's main pipeline mode, which
// contains different lifecycle control
// LynxLoadOption represents switches for Lynx's specific atomic abilities
public final class LynxLoadMeta {
  String url;
  byte[] binaryData;
  ByteBuffer byteBuffer;
  TemplateBundle bundle;
  TemplateData initialData;
  TemplateData globalProps;
  LynxLoadMode loadMode;
  int loadOptions;
  Map<String, String> lynxViewConfig;

  private LynxLoadMeta(String url, byte[] binaryData, ByteBuffer byteBuffer, TemplateBundle bundle,
      TemplateData initialData, TemplateData globalProps, LynxLoadMode loadMode, int loadOptions,
      Map<String, String> lynxViewConfig) {
    this.url = url;
    this.binaryData = binaryData;
    this.byteBuffer = byteBuffer;
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

  /**
   * Check whether the incoming byteBuffer is valid;
   *
   * @return True if byteBuffer is valid;
   */
  public boolean isByteBufferValid() {
    return this.byteBuffer != null;
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
    EnumSet<LynxLoadOption> set = EnumSet.noneOf(LynxLoadOption.class);
    if (enableDumpElementTree()) {
      set.add(LynxLoadOption.DUMP_ELEMENT);
    }
    if (enableRecycleTemplateBundle()) {
      set.add(LynxLoadOption.RECYCLE_TEMPLATE_BUNDLE);
    }
    if (enableProcessLayout()) {
      set.add(LynxLoadOption.PROCESS_LAYOUT_WITHOUT_UI_FLUSH);
    }
    if (renderForRecreateEngine()) {
      set.add(LynxLoadOption.RENDER_FOR_RECREATE_ENGINE);
    }
    return set;
  }

  public ByteBuffer getByteBuffer() {
    return byteBuffer;
  }

  public boolean enableDumpElementTree() {
    return (this.loadOptions & LynxLoadOption.DUMP_ELEMENT.id()) != 0;
  }

  public boolean enableRecycleTemplateBundle() {
    return (this.loadOptions & LynxLoadOption.RECYCLE_TEMPLATE_BUNDLE.id()) != 0;
  }

  public boolean enableProcessLayout() {
    return (this.loadOptions & LynxLoadOption.PROCESS_LAYOUT_WITHOUT_UI_FLUSH.id()) != 0;
  }

  private boolean renderForRecreateEngine() {
    return (this.loadOptions & LynxLoadOption.RENDER_FOR_RECREATE_ENGINE.id()) != 0;
  }

  public static class Builder {
    private String url;
    private byte[] binaryData;
    private TemplateBundle bundle;
    private TemplateData initialData;
    private TemplateData globalProps;
    private LynxLoadMode loadMode;
    private int loadOptions = 0;
    private ByteBuffer byteBuffer;
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
      this.loadOptions |= loadOption.id();
    }
    public void setLynxViewConfig(Map<String, String> lynxViewConfig) {
      this.lynxViewConfig = lynxViewConfig;
    }
    public void setByteBuffer(ByteBuffer buffer) {
      this.byteBuffer = buffer;
    }

    public LynxLoadMeta build() {
      return new LynxLoadMeta(this.url, this.binaryData, this.byteBuffer, this.bundle,
          this.initialData, this.globalProps, this.loadMode, this.loadOptions, this.lynxViewConfig);
    }
  }
}
