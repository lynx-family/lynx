// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.base;

/**
 * !!! Need to be aligned with native layer. !!!
 * !!! in file <logging.h> !!!
 * 0 Native:  C++ log
 * 1 JS:      JS log
 * 2 JS_EXT:  console.alog & console.Report log
 * 3 JAVA:    JAVA log
 */
public enum LogSource { Native, JS, JS_EXT, JAVA }
