// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package org.lynxsdk.extension

import groovy.transform.TupleConstructor

@TupleConstructor
class LynxExtensionInfo {
    String npmName
    File packageDir
    File manifestFile
    String androidPackageName
    String androidSourceDir
    File androidDir
    String projectPath

    String getProviderClassName() {
        "${androidPackageName}.LynxExtensionProviderImpl"
    }
}
