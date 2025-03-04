// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.resourceprovider.template;

import com.lynx.tasm.TemplateBundle;

public class TemplateProviderResult {
  private byte[] templateBinary = null;
  private TemplateBundle templateBundle = null;

  public byte[] getTemplateBinary() {
    return this.templateBinary;
  }

  public TemplateBundle getTemplateBundle() {
    return this.templateBundle;
  }

  private TemplateProviderResult() {}

  public static TemplateProviderResult fromBinary(byte[] binary) {
    TemplateProviderResult result = new TemplateProviderResult();
    result.templateBinary = binary;
    return result;
  }

  public static TemplateProviderResult fromTemplateBundle(TemplateBundle bundle) {
    TemplateProviderResult result = new TemplateProviderResult();
    result.templateBundle = bundle;
    return result;
  }
}
