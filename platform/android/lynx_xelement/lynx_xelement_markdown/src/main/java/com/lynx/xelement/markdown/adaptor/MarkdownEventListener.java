// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.markdown.adaptor;

import com.lynx.markdown.IMarkdownEventListener;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.event.LynxDetailEvent;
import com.lynx.tasm.utils.UIThreadUtils;

public class MarkdownEventListener implements IMarkdownEventListener {
  static final String EVENT_START = "drawStart";
  static final String EVENT_END = "drawEnd";
  static final String EVENT_ANIMATION_STEP = "animationStep";
  static final String EVENT_OVERFLOW = "overflow";
  static final String EVENT_LINK = "link";
  static final String EVENT_SELECTION_CHANGE = "selectionchange";
  static final String EVENT_CHILDREN_EXPOSE = "childrenexpose";
  static final String EVENT_PARSE_END = "parseEnd";
  static final String EVENT_IMAGE_TAP = "imageTap";
  static final String EVENT_TEXT_CLICK = "textClick";

  private final MarkdownEventContext mHost;

  public MarkdownEventListener(MarkdownEventContext host) {
    mHost = host;
  }

  @Override
  public void onParseEnd() {
    dispatchParseEnd(mHost.getShadowNode(), mHost.getParseEndContentID());
  }
  @Override
  public void onTextOverflow(int overflow) {
    dispatchOverflow(mHost.getShadowNode(), overflow == 0 ? "clip" : "ellipsis");
  }
  @Override
  public void onDrawStart() {
    dispatchSimpleEvent(mHost.getShadowNode(), EVENT_START);
  }
  @Override
  public void onDrawEnd() {
    dispatchSimpleEvent(mHost.getShadowNode(), EVENT_END);
  }
  @Override
  public void onAnimationStep(int animationStep, int maxAnimationStep) {
    dispatchAnimationStep(
        mHost.getShadowNode(), animationStep, maxAnimationStep, mHost.getContentLength());
  }
  @Override
  public void onLinkClicked(String url, String content) {
    dispatchLink(mHost.getShadowNode(), url, content);
  }
  @Override
  public void onImageClicked(String url) {
    dispatchImageTap(mHost.getShadowNode(), url);
  }
  @Override
  public void onSelectionChanged(int startIndex, int endIndex, int handle, int state) {
    dispatchSelectionChange(mHost.getShadowNode(), startIndex, endIndex, startIndex <= endIndex);
  }
  public static void dispatchSimpleEvent(final ShadowNode node, final String eventName) {
    final LynxDetailEvent event = new LynxDetailEvent(node.getSignature(), eventName);
    UIThreadUtils.runOnUiThread(() -> node.getContext().getEventEmitter().sendCustomEvent(event));
  }
  public static void dispatchAnimationStep(
      final ShadowNode node, final int step, final int maxStep, final int contentLength) {
    final LynxDetailEvent event = new LynxDetailEvent(node.getSignature(), EVENT_ANIMATION_STEP);
    event.addDetail("animationStep", step);
    event.addDetail("maxAnimationStep", maxStep);
    event.addDetail("contentLength", contentLength);
    UIThreadUtils.runOnUiThread(() -> node.getContext().getEventEmitter().sendCustomEvent(event));
  }
  public static void dispatchOverflow(final ShadowNode node, final String type) {
    final LynxDetailEvent event = new LynxDetailEvent(node.getSignature(), EVENT_OVERFLOW);
    event.addDetail("type", type);
    UIThreadUtils.runOnUiThread(() -> node.getContext().getEventEmitter().sendCustomEvent(event));
  }
  public static void dispatchLink(final ShadowNode node, final String link, final String content) {
    final LynxDetailEvent event = new LynxDetailEvent(node.getSignature(), EVENT_LINK);
    event.addDetail("url", link);
    event.addDetail("content", content);
    UIThreadUtils.runOnUiThread(() -> node.getContext().getEventEmitter().sendCustomEvent(event));
  }
  public static void dispatchSelectionChange(
      final ShadowNode node, final int start, final int end, final boolean isForward) {
    final LynxDetailEvent event = new LynxDetailEvent(node.getSignature(), EVENT_SELECTION_CHANGE);
    event.addDetail("start", start);
    event.addDetail("end", end);
    event.addDetail("direction", isForward ? "forward" : "backward");
    UIThreadUtils.runOnUiThread(() -> node.getContext().getEventEmitter().sendCustomEvent(event));
  }
  public static void dispatchParseEnd(final ShadowNode node, final String id) {
    final LynxDetailEvent event = new LynxDetailEvent(node.getSignature(), EVENT_PARSE_END);
    event.addDetail("id", id);
    UIThreadUtils.runOnUiThread(() -> node.getContext().getEventEmitter().sendCustomEvent(event));
  }
  public static void dispatchImageTap(final ShadowNode node, final String link) {
    final LynxDetailEvent event = new LynxDetailEvent(node.getSignature(), EVENT_IMAGE_TAP);
    event.addDetail("url", link);
    UIThreadUtils.runOnUiThread(() -> node.getContext().getEventEmitter().sendCustomEvent(event));
  }
  public static void dispatchTextClick(final ShadowNode node, final String id) {
    final LynxDetailEvent event = new LynxDetailEvent(node.getSignature(), EVENT_TEXT_CLICK);
    event.addDetail("id", id);
    UIThreadUtils.runOnUiThread(() -> node.getContext().getEventEmitter().sendCustomEvent(event));
  }
}
