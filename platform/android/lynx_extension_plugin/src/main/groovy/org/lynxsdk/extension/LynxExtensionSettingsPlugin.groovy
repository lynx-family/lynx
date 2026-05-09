// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package org.lynxsdk.extension

import org.gradle.api.Plugin
import org.gradle.api.initialization.Settings

class LynxExtensionSettingsPlugin implements Plugin<Settings> {
    @Override
    void apply(Settings settings) {
        List<LynxExtensionInfo> extensions = LynxExtensionScanner.scan(settings.settingsDir)
        settings.extensions.extraProperties.set('lynxAutolinkExtensions', extensions)
        extensions.each { LynxExtensionInfo extension ->
            settings.include(extension.projectPath)
            settings.project(extension.projectPath).projectDir = extension.androidDir
        }
    }
}
