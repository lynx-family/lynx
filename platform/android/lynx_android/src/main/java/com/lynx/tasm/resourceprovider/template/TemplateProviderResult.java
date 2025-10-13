// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.resourceprovider.template;

import com.lynx.tasm.TemplateBundle;
import java.nio.ByteBuffer;

public class TemplateProviderResult {
  private byte[] templateBinary = null;
  private ByteBuffer templateBuffer = null;
  private TemplateBundle templateBundle = null;

  /**
   * @return binary content inside the TemplateProviderResult
   */
  public byte[] getTemplateBinary() {
    return this.templateBinary;
  }

  /**
   * @return templateBundle instance inside the TemplateProviderResult
   */
  public TemplateBundle getTemplateBundle() {
    return this.templateBundle;
  }

  /**
   * @return ByteBuffer instance inside the TemplateProviderResult
   */
  public ByteBuffer getTemplateBuffer() {
    return this.templateBuffer;
  }

  private TemplateProviderResult() {}

  /**
   * Constructs a TemplateProviderResult result by ByteArray
   * @param binary ByteArray
   * @return TemplateProviderResult instance
   */
  public static TemplateProviderResult fromBinary(byte[] binary) {
    TemplateProviderResult result = new TemplateProviderResult();
    result.templateBinary = binary;
    return result;
  }

  /**
   * Constructs a TemplateProviderResult result by TemplateBundle
   * @param bundle TemplateBundle instance
   * @return TemplateProviderResult instance
   */
  public static TemplateProviderResult fromTemplateBundle(TemplateBundle bundle) {
    TemplateProviderResult result = new TemplateProviderResult();
    result.templateBundle = bundle;
    return result;
  }

  /**
   * Constructs a TemplateProviderResult result by ByteBuffer
   * @param byteBuffer input bytebuffer
   * @return TemplateProviderResult instance
   */
  public static TemplateProviderResult fromBuffer(ByteBuffer byteBuffer) {
    TemplateProviderResult result = new TemplateProviderResult();
    result.templateBuffer = byteBuffer;
    return result;
  }
}
