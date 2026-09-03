// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package org.lynxsdk.library

import groovy.transform.TupleConstructor

@TupleConstructor
class LynxNodeApiAddonInfo {
    String name
    String libraryName
    File jniLibsDir
    boolean required

    boolean hasPrebuiltLibrary() {
        jniLibsDir != null
    }

    String getSharedLibraryName() {
        "lib${libraryName}.so"
    }
}

@TupleConstructor
class LynxLibraryInfo {
    // Human-readable identifier used for sorting and diagnostics only; uniqueness
    // is enforced by providerClassName, not this. For node_modules it is the npm
    // name, for local_project the Gradle project path. TODO: consider renaming to
    // a source-neutral name (e.g. displayName) now that multiple sources exist.
    String npmName
    File packageDir
    File manifestFile
    String androidPackageName
    String androidSourceDir
    File androidDir
    String projectPath
    String providerClassName
    List<LynxNodeApiAddonInfo> nodeApiAddons = []
    // Which discovery source produced this library. Determines downstream
    // handling, e.g. only node_modules libraries need the build plugin to add
    // `implementation project(...)`; local_project libraries are depended on by
    // the host itself, and aar libraries are already resolved dependencies.
    String source = LynxLibraryScanner.SOURCE_NODE_MODULES

    String getProviderClassName() {
        "${androidPackageName}.LynxLibraryProviderImpl"
    }
}
