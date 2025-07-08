// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export class LynxLoadOption {
  // Enable to dump a TemplateBundle with element tree after first load through
  // onTemplateBundleReady in LynxViewClient
  private enableDumpElement: boolean = false;
  // Enable to provide a reusable TemplateBundle after template is decoding
  // through onTemplateBundleReady in LynxViewClient
  private enableRecycleTemplateBundle: boolean = false;

  constructor(enableDumpElement: boolean = false, enableRecycleTemplateBundle: boolean = false) {
    this.enableDumpElement = enableDumpElement;
    this.enableRecycleTemplateBundle = enableRecycleTemplateBundle;
  }

  public isEnableDumpElement(): boolean {
    return this.enableDumpElement;
  }

  public isEnableRecycleTemplateBundle(): boolean {
    return this.enableRecycleTemplateBundle;
  }
}