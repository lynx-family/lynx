// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.base;

public interface IBaseNativeLibraryLoader {
  void loadLibrary(String libName) throws UnsatisfiedLinkError;
}
