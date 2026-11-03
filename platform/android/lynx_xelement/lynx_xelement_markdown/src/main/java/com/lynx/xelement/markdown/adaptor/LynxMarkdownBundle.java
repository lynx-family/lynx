// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.markdown.adaptor;

import com.lynx.markdown.MarkdownMeasurer;
import com.lynx.xelement.markdown.LynxUIMarkdownShadowNode;

public final class LynxMarkdownBundle {
  public final MarkdownMeasurer mMarkdownMeasurer;
  public final LynxUIMarkdownShadowNode mShadowNode;
  public final MarkdownResourceContext mResourceContext;
  public final MarkdownResourceLoader mResourceLoader;
  public final long mMeasuredSize;
  public final String mContentID;
  public final boolean mExposeLinks;

  public LynxMarkdownBundle(MarkdownMeasurer markdownMeasurer, LynxUIMarkdownShadowNode shadowNode,
      MarkdownResourceContext resourceContext, MarkdownResourceLoader resourceLoader,
      long measuredSize) {
    mMarkdownMeasurer = markdownMeasurer;
    mShadowNode = shadowNode;
    mResourceContext = resourceContext;
    mResourceLoader = resourceLoader;
    mMeasuredSize = measuredSize;
    mContentID = shadowNode.getParseEndContentID();
    mExposeLinks = shadowNode.exposesLinks();
  }
}
