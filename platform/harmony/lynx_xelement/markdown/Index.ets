// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import markdown from 'liblynx_xelement_markdown.so';

export class XElementMarkdown {
  private static initialized: boolean = false;

  static initialize(): void {
    if (XElementMarkdown.initialized) {
      return;
    }
    markdown.initMarkdown();
    XElementMarkdown.initialized = true;
  }
}
