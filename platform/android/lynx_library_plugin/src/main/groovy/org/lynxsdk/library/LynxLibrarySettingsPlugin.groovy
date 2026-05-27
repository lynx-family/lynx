// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package org.lynxsdk.library

import org.gradle.api.Plugin
import org.gradle.api.initialization.Settings

class LynxLibrarySettingsPlugin implements Plugin<Settings> {
    @Override
    void apply(Settings settings) {
        List<LynxLibraryInfo> libraries = LynxLibraryScanner.scan(settings.settingsDir)
        settings.extensions.extraProperties.set('lynxAutolinkLibraries', libraries)
        libraries.each { LynxLibraryInfo library ->
            settings.include(library.projectPath)
            settings.project(library.projectPath).projectDir = library.androidDir
        }
    }
}
