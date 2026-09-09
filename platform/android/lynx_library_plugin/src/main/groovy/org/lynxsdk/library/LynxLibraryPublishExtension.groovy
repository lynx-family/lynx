// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package org.lynxsdk.library

// DSL for org.lynxsdk.lynx.library-publish.
//
// The only knob is an optional manifest path. When unset, the plugin reads
// lynx.lib.json from the project root; set it to point at a manifest elsewhere.
class LynxLibraryPublishExtension {
    File manifest
}
