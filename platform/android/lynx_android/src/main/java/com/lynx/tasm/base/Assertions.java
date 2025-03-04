// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.base;
import androidx.annotation.Nullable;
import java.util.List;
import java.util.Map;

public class Assertions {
  public Assertions() {}

  public static <T> T assumeNotNull(@Nullable T var0) {
    return var0;
  }

  public static <T> T assumeNotNull(@Nullable T var0, String var1) {
    return var0;
  }

  public static <T> T assertNotNull(@Nullable T var0) {
    if (var0 == null) {
      throw new AssertionError();
    } else {
      return var0;
    }
  }

  public static <T> T assertNotNull(@Nullable T var0, String var1) {
    if (var0 == null) {
      throw new AssertionError(var1);
    } else {
      return var0;
    }
  }

  public static <T> T getAssumingNotNull(List<T> var0, int var1) {
    return var0.get(var1);
  }

  public static <T> T getAssertingNotNull(List<T> var0, int var1) {
    assertCondition(0 <= var1 && var1 < var0.size());
    return assertNotNull(var0.get(var1));
  }

  public static <K, V> V getAssumingNotNull(Map<K, V> var0, K var1) {
    return var0.get(var1);
  }

  public static <K, V> V getAssertingNotNull(Map<K, V> var0, K var1) {
    assertCondition(var0.containsKey(var1));
    return assertNotNull(var0.get(var1));
  }

  public static void assumeCondition(boolean var0) {}

  public static void assumeCondition(boolean var0, String var1) {}

  public static void assertCondition(boolean var0) {
    if (!var0) {
      throw new AssertionError();
    }
  }

  public static void assertCondition(boolean var0, String var1) {
    if (!var0) {
      throw new AssertionError(var1);
    }
  }

  public static AssertionError assertUnreachable() {
    throw new AssertionError();
  }

  public static AssertionError assertUnreachable(String var0) {
    throw new AssertionError(var0);
  }

  public static AssertionError assertUnreachable(Exception var0) {
    throw new AssertionError(var0);
  }
}
