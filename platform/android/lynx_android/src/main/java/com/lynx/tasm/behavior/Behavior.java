// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import com.lynx.tasm.BehaviorClassWarmer;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.behavior.ui.LynxFlattenUI;
import com.lynx.tasm.behavior.ui.LynxUI;

public class Behavior {
  private String mName;
  private final boolean mFlatten;
  private final boolean mCreateAsync;
  // TODO: If additional fields are necessary, including a metadata field is required
  // to prevent the constructor from becoming overly complex.
  private final boolean mNeedProcessDirection;

  public Behavior(String name) {
    this(name, false, false, false);
  }

  public Behavior(String name, boolean flatten) {
    this(name, flatten, false, false);
  }

  public Behavior(String name, boolean flatten, boolean createAsync) {
    this(name, flatten, createAsync, false);
  }

  /**
   * Constructing functions of Behavior.
   * In a page, can support the asynchronous creation of components asynchronously
   * and the synchronous creation of components that do not support asynchrony,
   * so if the Behavior support create async, the value of createAsync could be 'true'.
   *
   * @param name Behavior name
   * @param flatten
   * @param createAsync whether the behavior supports create async
   * @param needProcessDirection whether the behavior need direction-related prop to work properly
   *     in RTL
   */
  public Behavior(String name, boolean flatten, boolean createAsync, boolean needProcessDirection) {
    mName = name;
    mFlatten = flatten;
    mCreateAsync = createAsync;
    mNeedProcessDirection = needProcessDirection;
  }

  public boolean supportCreateAsync() {
    return mCreateAsync;
  }

  public boolean needProcessDirection() {
    return mNeedProcessDirection;
  }

  public LynxUI createUIWithParams(LynxContext context, Object params) {
    return createUI(context);
  }

  public LynxFlattenUI createFlattenUIWithParams(LynxContext context, Object params) {
    return createFlattenUI(context);
  }

  public LynxUI createUI(LynxContext context) {
    // It means this is a virtual node without real ui if subclass do not override this method
    throw new RuntimeException(mName + " is a virtual node, do not have real ui!");
  }

  public LynxUI createUIFiber(LynxContext context) {
    // It means this is a virtual node without real ui if subclass do not override this method
    throw new RuntimeException(mName + " is a virtual node, do not have real ui!");
  }

  public LynxFlattenUI createFlattenUI(LynxContext context) {
    return null;
  }

  public LynxFlattenUI createFlattenUIFiber(LynxContext context) {
    return null;
  }

  public ShadowNode createShadowNode() {
    return null;
  }

  public BehaviorClassWarmer createClassWarmer() {
    return null;
  }

  public final boolean supportUIFlatten() {
    return mFlatten;
  }

  public String getName() {
    return mName;
  }

  @Override
  public String toString() {
    return "[" + this.getClass().getSimpleName() + " - " + mName + "]";
  }
}
