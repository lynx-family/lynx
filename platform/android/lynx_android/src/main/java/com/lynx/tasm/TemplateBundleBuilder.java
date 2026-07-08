// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

/**
 * @apidoc
 * @brief Builds a `TemplateBundle` from separated template sources.
 */
public final class TemplateBundleBuilder {
  @Nullable private String mainThreadScript;
  @Nullable private String backgroundThreadScript;

  private TemplateBundleBuilder() {}

  /**
   * @apidoc
   * @brief Creates a `TemplateBundleBuilder`.
   */
  public static TemplateBundleBuilder create() {
    return new TemplateBundleBuilder();
  }

  /**
   * @apidoc
   * @brief Sets the main-thread script source.
   */
  public TemplateBundleBuilder main(@Nullable String script) {
    mainThreadScript = script;
    return this;
  }

  /**
   * @apidoc
   * @brief Sets the background script source.
   */
  public TemplateBundleBuilder background(@Nullable String script) {
    backgroundThreadScript = script;
    return this;
  }

  /**
   * @apidoc
   * @brief Builds a `TemplateBundle`.
   */
  @NonNull
  public TemplateBundle build() {
    return TemplateBundle.build(mainThreadScript, backgroundThreadScript, null);
  }
}
