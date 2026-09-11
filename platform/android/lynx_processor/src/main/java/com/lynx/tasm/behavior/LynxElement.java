// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior;

import static java.lang.annotation.RetentionPolicy.RUNTIME;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.Target;

/**
 * Compile-time mirror used by LynxProcessor. Keep this in sync with the public
 * LynxElement annotation.
 */
@Retention(RUNTIME)
@Target(ElementType.TYPE)
public @interface LynxElement {
  String name();
  boolean isCreateAsync() default false;
  boolean needProcessDirection() default false;
  boolean supportFragmentLayerRender() default false;
  Class<?> fragmentLayerRendererHost() default void.class;
  /**
   * Whether this component can render ordinary child content through Fragment Layer
   * display lists without requiring legacy child UIs. Opt in only when the component's
   * child mounting and drawing are compatible with the Fragment Layer display-list path.
   * Enabling this does not remove the component's own UI, and does not automatically
   * change the behavior of extended child components. It is independent from
   * supportFragmentLayerRender and fragmentLayerRendererHost.
   */
  boolean supportFragmentLayerChildren() default false;
}
