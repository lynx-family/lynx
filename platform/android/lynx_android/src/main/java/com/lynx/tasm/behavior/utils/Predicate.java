// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.utils;

/**
 * Represents a predicate (boolean-valued function) of one argument.
 * <p>
 * java.util.function.Predicate requires API 24. This interface
 * serves as an alternative.
 */
public interface Predicate<T> {
  /**
   * Evaluates this predicate on the given argument.
   *
   * @param var1 the input argument to test
   * @return true if the input argument matches the predicate, otherwise false
   */
  boolean test(T var1);
}
