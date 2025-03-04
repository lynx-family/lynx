// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.jsbridge.jsi;

import androidx.annotation.Keep;

/**
 * `ILynxJSIObject` is a cross-platform data structure whose fields can be directly accessed by
 * JavaScript. All fields that need to be accessed by JS must be annotated with `@LynxJSProperty`
 * and the class itself must be `keep` to ensure accessibility,
 */
@Keep
public interface ILynxJSIObject {}
