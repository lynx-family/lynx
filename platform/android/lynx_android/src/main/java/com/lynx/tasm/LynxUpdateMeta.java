// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

/**
 * Class for update data & globalProps at once.
 */
public final class LynxUpdateMeta {
  private TemplateData updatedData;
  private TemplateData updatedGlobalProps;

  private LynxUpdateMeta(TemplateData updateData, TemplateData updateGlobalProps) {
    this.updatedData = updateData;
    this.updatedGlobalProps = updateGlobalProps;
  }

  public TemplateData getUpdatedData() {
    return this.updatedData;
  }

  public TemplateData getUpdatedGlobalProps() {
    return this.updatedGlobalProps;
  }

  public static class Builder {
    private TemplateData updatedData;
    private TemplateData updatedGlobalProps;

    public Builder setUpdatedData(TemplateData updatedData) {
      this.updatedData = updatedData;
      return this;
    }

    public Builder setUpdatedGlobalProps(TemplateData updatedGlobalProps) {
      this.updatedGlobalProps = updatedGlobalProps;
      return this;
    }

    public LynxUpdateMeta build() {
      return new LynxUpdateMeta(updatedData, updatedGlobalProps);
    }
  }
}
